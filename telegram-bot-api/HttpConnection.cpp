//
// Copyright Aliaksei Levin (levlam@telegram.org), Arseny Smirnov (arseny30@gmail.com) 2014-2026
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "telegram-bot-api/HttpConnection.h"

#include "telegram-bot-api/Query.h"

#include "td/net/HttpHeaderCreator.h"

#include "td/utils/common.h"
#include "td/utils/filesystem.h"
#include "td/utils/JsonBuilder.h"
#include "td/utils/logging.h"
#include "td/utils/PathView.h"
#include "td/utils/Parser.h"
#include "td/utils/port/path.h"
#include "td/utils/port/Stat.h"
#include "td/utils/Promise.h"
#include "td/utils/SliceBuilder.h"

namespace telegram_bot_api {

namespace {

bool is_valid_relative_file_path(td::Slice path) {
  if (path.empty() || td::PathView(path).is_absolute()) {
    return false;
  }

  td::string current;
  current.reserve(path.size());
  for (auto c : path) {
    if (c == '/' || c == '\\') {
      if (current.empty() || current == "." || current == "..") {
        return false;
      }
      current.clear();
      continue;
    }
    current.push_back(c);
  }

  return !current.empty() && current != "." && current != "..";
}

td::string get_token_directory(td::Slice token, bool is_test_dc, bool allow_colon_in_filenames) {
  td::string suffix = PSTRING() << token << (is_test_dc ? ":T" : "") << TD_DIR_SLASH;
  if (!allow_colon_in_filenames) {
    for (auto &c : suffix) {
      if (c == ':') {
        c = '~';
      }
    }
  }
  return suffix;
}

bool is_inside_directory(td::Slice child_path, td::Slice directory_path) {
  if (!td::begins_with(child_path, directory_path)) {
    return false;
  }
  return child_path.size() > directory_path.size();
}

td::Slice get_content_type(td::Slice path) {
  static constexpr td::Slice jpeg = "image/jpeg";
  static constexpr td::Slice png = "image/png";
  static constexpr td::Slice gif = "image/gif";
  static constexpr td::Slice webp = "image/webp";
  static constexpr td::Slice mp4 = "video/mp4";
  static constexpr td::Slice pdf = "application/pdf";
  static constexpr td::Slice json = "application/json";
  static constexpr td::Slice plain = "text/plain; charset=utf-8";
  static constexpr td::Slice octet = "application/octet-stream";

  td::string extension = td::PathView(path).extension().str();
  td::to_lower_inplace(extension);
  if (extension == "jpg" || extension == "jpeg") {
    return jpeg;
  }
  if (extension == "png") {
    return png;
  }
  if (extension == "gif") {
    return gif;
  }
  if (extension == "webp") {
    return webp;
  }
  if (extension == "mp4") {
    return mp4;
  }
  if (extension == "pdf") {
    return pdf;
  }
  if (extension == "json") {
    return json;
  }
  if (extension == "txt" || extension == "log") {
    return plain;
  }
  return octet;
}

}  // namespace

void HttpConnection::handle(td::unique_ptr<td::HttpQuery> http_query,
                            td::ActorOwn<td::HttpInboundConnection> connection) {
  CHECK(connection_.empty());
  connection_ = std::move(connection);

  LOG(DEBUG) << "Handle " << *http_query;
  td::Parser url_path_parser(http_query->url_path_);
  if (url_path_parser.peek_char() != '/') {
    return send_http_error(404, "Not Found: absolute URI is specified in the Request-Line");
  }

  if (url_path_parser.try_skip("/file/")) {
    return handle_file_request(url_path_parser);
  }

  bool is_login = false;
  bool is_user = false;
  if (url_path_parser.try_skip("/bot")) {
  } else if (url_path_parser.try_skip("/userlogin") || url_path_parser.try_skip("/userLogin")) {
    is_user = true;
    is_login = true;
  } else if (url_path_parser.try_skip("/user")) {
    is_user = true;
  } else {
    return send_http_error(404, "Not Found");
  }

  td::MutableSlice token;
  bool is_test_dc = false;
  td::MutableSlice method;
  if (!is_login) {
    token = url_path_parser.read_till('/');
    is_test_dc = false;
    if (url_path_parser.try_skip("/test")) {
      is_test_dc = true;
    }
    url_path_parser.skip('/');
    if (url_path_parser.status().is_error()) {
      return send_http_error(404, "Not Found");
    }

    method = url_path_parser.data();
  }


  auto query = td::make_unique<Query>(std::move(http_query->container_), token, is_user, is_test_dc, method,
                                       std::move(http_query->args_), std::move(http_query->headers_),
                                       std::move(http_query->files_), shared_data_, http_query->peer_address_, false);

  auto promise = td::PromiseCreator::lambda([actor_id = actor_id(this)](td::Result<td::unique_ptr<Query>> r_query) {
    send_closure(actor_id, &HttpConnection::on_query_finished, std::move(r_query));
  });
  auto promised_query = PromisedQueryPtr(query.release(), PromiseDeleter(std::move(promise)));
  if (is_login) {
    send_closure(client_manager_, &ClientManager::user_login, std::move(promised_query));
  } else {
    send_closure(client_manager_, &ClientManager::send, std::move(promised_query));
  }
}

void HttpConnection::handle_file_request(td::Parser &url_path_parser) {
  bool is_user = false;
  if (url_path_parser.try_skip("bot")) {
  } else if (url_path_parser.try_skip("user")) {
    is_user = true;
  } else {
    return send_http_error(404, "Not Found");
  }

  if (is_user && !parameters_->allow_users_) {
    return send_http_error(405, "Method Not Allowed: Users are not allowed to use the api");
  }

  auto token = url_path_parser.read_till('/');
  auto token_str = token.str();
  auto colon_pos = token_str.find(':');
  if (token.empty() || token[0] == '0' || token.size() > 80u || token_str.find('/') != td::string::npos ||
      colon_pos == td::string::npos) {
    return send_http_error(401, "Unauthorized: invalid token specified");
  }

  auto r_user_id = td::to_integer_safe<td::int64>(token.substr(0, colon_pos));
  if (r_user_id.is_error() || !token_range_(r_user_id.ok())) {
    return send_http_error(421, "Misdirected Request: unallowed token specified");
  }
  auto user_id = r_user_id.ok();
  if (user_id <= 0 || user_id >= (static_cast<td::int64>(1) << 54)) {
    return send_http_error(401, "Unauthorized: invalid token specified");
  }

  bool is_test_dc = false;
  if (url_path_parser.try_skip("/test")) {
    is_test_dc = true;
  }
  url_path_parser.skip('/');
  if (url_path_parser.status().is_error()) {
    return send_http_error(404, "Not Found");
  }

  auto relative_path = url_path_parser.data();
  if (!is_valid_relative_file_path(relative_path)) {
    return send_http_error(400, "Bad Request: invalid file path specified");
  }

  auto token_with_dc = PSTRING() << token << (is_test_dc ? ":T" : "");
  if (shared_data_->user_db_->isset(token_with_dc) != is_user) {
    return send_http_error(400, "Bad Request: Please use the correct api endpoint for bots or users");
  }

  // downloaded files live under files_directory_ since --files-dir was added; it equals
  // working_directory_ unless the option is set
  auto base_dir = parameters_->files_directory_ +
                  get_token_directory(token, is_test_dc, parameters_->allow_colon_in_filenames_);
  auto r_base_dir_real = td::realpath(base_dir);
  if (r_base_dir_real.is_error()) {
    return send_http_error(404, "Not Found");
  }
  auto base_dir_real = r_base_dir_real.move_as_ok();
  if (base_dir_real.back() != TD_DIR_SLASH) {
    base_dir_real.push_back(TD_DIR_SLASH);
  }

  auto requested_path = PSTRING() << base_dir << relative_path;
  auto r_file_real = td::realpath(requested_path);
  if (r_file_real.is_error()) {
    return send_http_error(404, "Not Found");
  }
  auto file_real = r_file_real.move_as_ok();
  if (!is_inside_directory(file_real, base_dir_real)) {
    return send_http_error(403, "Forbidden: invalid file path specified");
  }

  auto r_file_stat = td::stat(file_real);
  if (r_file_stat.is_error()) {
    return send_http_error(404, "Not Found");
  }
  auto file_stat = r_file_stat.move_as_ok();
  if (!file_stat.is_reg_) {
    return send_http_error(404, "Not Found");
  }

  auto r_content = td::read_file(file_real);
  if (r_content.is_error()) {
    LOG(INFO) << "Failed to read file " << file_real << ": " << r_content.error();
    return send_http_error(404, "Not Found");
  }
  auto content = r_content.move_as_ok();
  send_response(200, std::move(content), get_content_type(file_real), 0);
}

void HttpConnection::on_query_finished(td::Result<td::unique_ptr<Query>> r_query) {
  LOG_CHECK(r_query.is_ok()) << r_query.error();

  auto query = r_query.move_as_ok();
  send_response(query->http_status_code(), std::move(query->answer()), query->retry_after());
}

void HttpConnection::send_response(int http_status_code, td::BufferSlice &&content, int retry_after) {
  send_response(http_status_code, std::move(content), "application/json", retry_after);
}

void HttpConnection::send_response(int http_status_code, td::BufferSlice &&content, td::Slice content_type,
                                   int retry_after) {
  td::HttpHeaderCreator hc;
  hc.init_status_line(http_status_code);
  hc.set_keep_alive();
  hc.set_content_type(content_type);
  if (retry_after > 0) {
    hc.add_header("Retry-After", PSLICE() << retry_after);
  }
  hc.set_content_size(content.size());

  auto r_header = hc.finish();
  LOG(DEBUG) << "Response headers: " << r_header.ok();
  if (r_header.is_error()) {
    LOG(ERROR) << "Bad response headers";
    send_closure(std::move(connection_), &td::HttpInboundConnection::write_error, r_header.move_as_error());
    return;
  }
  LOG(DEBUG) << "Send result: " << content;

  send_closure(connection_, &td::HttpInboundConnection::write_next_noflush, td::BufferSlice(r_header.ok()));
  send_closure(connection_, &td::HttpInboundConnection::write_next_noflush, std::move(content));
  send_closure(std::move(connection_), &td::HttpInboundConnection::write_ok);
}

void HttpConnection::send_http_error(int http_status_code, td::CSlice description) {
  send_response(http_status_code, td::json_encode<td::BufferSlice>(JsonQueryError(http_status_code, description)), 0);
}

}  // namespace telegram_bot_api
