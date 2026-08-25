//
// Copyright Aliaksei Levin (levlam@telegram.org), Arseny Smirnov (arseny30@gmail.com) 2014-2026
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "telegram-bot-api/ClientManager.h"
#include "telegram-bot-api/Query.h"

#include "td/net/HttpInboundConnection.h"
#include "td/net/HttpQuery.h"

#include "td/actor/actor.h"

#include "td/utils/buffer.h"
#include "td/utils/Slice.h"
#include "td/utils/Status.h"

#include <memory>

namespace telegram_bot_api {

struct SharedData;

class HttpConnection final : public td::HttpInboundConnection::Callback {
 public:
  HttpConnection(td::ActorId<ClientManager> client_manager, std::shared_ptr<const ClientParameters> parameters,
                 std::shared_ptr<SharedData> shared_data, ClientManager::TokenRange token_range)
      : client_manager_(client_manager)
      , parameters_(std::move(parameters))
      , shared_data_(std::move(shared_data))
      , token_range_(token_range) {
  }

  void handle(td::unique_ptr<td::HttpQuery> http_query, td::ActorOwn<td::HttpInboundConnection> connection) final;

 private:
  td::ActorId<ClientManager> client_manager_;
  std::shared_ptr<const ClientParameters> parameters_;
  td::ActorOwn<td::HttpInboundConnection> connection_;
  std::shared_ptr<SharedData> shared_data_;
  ClientManager::TokenRange token_range_;

  void hangup() final {
    connection_.release();
    stop();
  }

  void on_query_finished(td::Result<td::unique_ptr<Query>> r_query);

  void handle_file_request(td::Parser &url_path_parser);

  void send_response(int http_status_code, td::BufferSlice &&content, int retry_after);

  void send_response(int http_status_code, td::BufferSlice &&content, td::Slice content_type, int retry_after);

  void send_http_error(int http_status_code, td::CSlice description);
};

}  // namespace telegram_bot_api
