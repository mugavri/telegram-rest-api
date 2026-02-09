# Telegram Rest API

A REST API server for Telegram, based on a fork of [TDLight Telegram Bot API](https://github.com/tdlight-team/tdlight-telegram-bot-api). Features 35+ extended API methods, user mode support, proxy management, message analytics, and more.

Built on top of **TDLib 1.8.61** — fully compatible with the official [Telegram Bot API](https://core.telegram.org/bots/api).

## Table of Contents
- [What's Different](#whats-different)
- [Added API Methods](#added-api-methods)
  - [General Methods (Bot & User)](#general-methods-bot--user)
  - [Proxy Management](#proxy-management)
  - [User-Mode Methods](#user-mode-methods)
- [Modified Features](#modified-features)
  - [Modified Methods](#modified-methods)
  - [Extended Objects](#extended-objects)
- [User Mode](#user-mode)
- [Command Line Parameters](#command-line-parameters)
- [Docker & Environment Variables](#docker--environment-variables)
- [Installation](#installation)
- [Dependencies](#dependencies)
- [Usage](#usage)
- [Documentation](#documentation)
- [Moving a Bot to a Local Server](#switching)
- [Moving a Bot Between Local Servers](#moving)
- [License](#license)

---

<a name="whats-different"></a>
## What's Different

| Feature | Official Bot API | This Fork |
|---------|-----------------|-----------|
| User mode (userbots) | No | Yes |
| Proxy management API | No | Yes (MTProto, SOCKS5, HTTP) |
| Custom API methods | None | 35+ new methods |
| Message database | No | Yes (local caching) |
| Extended statistics | Basic | Enhanced (message stats, graphs, public forwards) |
| Extended objects | Standard | Extra fields on Message, Chat, User, ChatMember |
| Invite link analytics | Limited | Comprehensive |
| Message range deletion | No | Yes |

---

<a name="added-api-methods"></a>
## Added API Methods

<a name="general-methods-bot--user"></a>
### General Methods (Bot & User)

These methods work for both bot tokens and user tokens.

---

#### `getMessageInfo`
Get information about a specific message.

**Access:** Bot & User

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `chat_id` | Integer/String | Yes | Chat identifier |
| `message_id` | Integer | Yes | Message identifier |

**Returns:** `Message` object

---

#### `getMessage`
Get comprehensive message data including the message itself, its properties, statistics, public forwards, thread info, viewers, and available reactions.

**Access:** Bot & User

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `chat_id` | Integer/String | Yes | Chat identifier |
| `message_id` | Integer | Yes | Message identifier |

**Returns:** `fullMessage` object:

| Field | Type | Description |
|-------|------|-------------|
| `chat_id` | Integer | Chat identifier |
| `message_id` | Integer | Message identifier |
| `message` | Message | The full message object |
| `properties` | Object | Message capability flags (see below) |
| `statistics` | Object | Message statistics (interaction & reaction graphs) |
| `public_forwards` | Object | Public forwards (`total_count`, `next_offset`, `forwards[]`) |
| `thread_info` | Object | Thread information (if applicable) |
| `thread_messages` | Array of Message | Thread messages (if applicable) |
| `viewers` | Array of Object | Message viewers (`user`, `date`) |
| `available_reactions` | Object | Available reactions for this message |

**Properties fields:**
`can_add_tasks`, `can_be_copied`, `can_be_copied_to_secret_chat`, `can_be_deleted_only_for_self`, `can_be_deleted_for_all_users`, `can_be_edited`, `can_be_forwarded`, `can_be_paid`, `can_be_pinned`, `can_be_replied`, `can_be_replied_in_another_chat`, `can_be_saved`, `can_be_shared_in_story`, `can_edit_media`, `can_edit_scheduling_state`, `can_get_author`, `can_get_embedding_code`, `can_get_link`, `can_get_media_timestamp_links`, `can_get_message_thread`, `can_get_read_date`, `can_get_statistics`, `can_get_video_advertisements`, `can_get_viewers`, `can_mark_tasks_as_done`, `can_recognize_speech`, `can_report_chat`, `can_report_reactions`, `can_report_supergroup_spam`, `can_set_fact_check`, `need_show_statistics`

---

#### `getParticipants`
Get the member list of a supergroup or channel.

**Access:** Bot & User

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `chat_id` | Integer/String | Yes | Chat identifier |
| `filter` | String | No | Filter type: `members`, `admins`, `administrators`, `restricted`, `banned`, `bots` |
| `offset` | Integer | No | Number of users to skip |
| `limit` | Integer | No | Max number of users to return (up to 200) |

**Returns:** Array of `ChatMember` objects

---

#### `deleteMessages`
Delete messages in a range. Works on supergroups only.

**Access:** Bot & User

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `chat_id` | Integer/String | Yes | Chat identifier |
| `start` | Integer | Yes | First message ID in range |
| `end` | Integer | Yes | Last message ID in range |

`start` must be less than `end`. Both must be positive non-zero numbers.
Max messages per call is determined by `--max-batch-operations` (default 10000). Recommended: no more than 200 per call.

**Returns:** `true` (always, even if some messages couldn't be deleted)

---

#### `ping`
Send an MTProto ping to Telegram servers. Useful for measuring latency.

**Access:** Bot & User

**Parameters:** None

**Returns:** Ping delay in seconds as `string`

---

<a name="proxy-management"></a>
### Proxy Management

These methods work for both bot tokens and user tokens.

---

#### `getProxies`
Get all configured proxies.

**Access:** Bot & User

**Parameters:** None

**Returns:** Array of proxy objects

---

#### `addProxy`
Add a new proxy.

**Access:** Bot & User

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `server` | String | Yes | Proxy server address |
| `port` | Integer | Yes | Proxy port (1–65535) |
| `type` | String | Yes | Proxy type: `mtproto`, `socks5`, or `http` |
| `secret` | String | MTProto only | Proxy secret (required for `mtproto` type) |
| `username` | String | No | Username (for `socks5` and `http`) |
| `password` | String | No | Password (for `socks5` and `http`) |
| `http_only` | Boolean | No | HTTP-only mode (for `http` type) |

**Returns:** Added proxy object

---

#### `deleteProxy`
Delete a proxy by ID.

**Access:** Bot & User

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `proxy_id` | Integer | Yes | Proxy identifier |

**Returns:** `true` on success

---

#### `enableProxy`
Enable a specific proxy.

**Access:** Bot & User

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `proxy_id` | Integer | Yes | Proxy identifier |

**Returns:** `true` on success

---

#### `disableProxy`
Disable all proxies.

**Access:** Bot & User

**Parameters:** None

**Returns:** `true` on success

---

<a name="user-mode-methods"></a>
### User-Mode Methods

These methods require [User Mode](#user-mode) — they only work with user tokens (`/user{token}/`).

---

#### `getChats`
Get the main chat list.

**Access:** User only

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `limit` | Integer | No | Max number of chats (default 100, max 100) |

**Returns:** Array of chat objects

---

#### `getCommonChats`
Get chats in common with another user.

**Access:** User only

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `user_id` | Integer | Yes | User identifier |
| `offset_chat_id` | Integer | No | Chat ID to offset from (default 0) |
| `limit` | Integer | No | Max number of chats (default 100, max 100) |

**Returns:** Array of chat objects

---

#### `getInactiveChats`
Get inactive supergroup and channel chats.

**Access:** User only

**Parameters:** None

**Returns:** Array of chat objects

---

#### `searchPublicChats`
Search for public chats by query.

**Access:** User only

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `query` | String | Yes | Search query |

**Returns:** Array of chat objects

---

#### `getChatSimilarChats`
Get chats similar to a given chat.

**Access:** User only

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `chat_id` | Integer/String | Yes | Chat identifier |

**Returns:** Array of chat objects

---

#### `joinChat`
Join a chat by ID or invite link.

**Access:** User only

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `chat_id` | Integer/String | No* | Chat identifier |
| `invite_link` | String | No* | Invite link |

*At least one of `chat_id` or `invite_link` must be provided.

**Returns:** `true` on success

---

#### `addChatMembers`
Add a member to a chat.

**Access:** User only

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `chat_id` | Integer/String | Yes | Chat identifier |
| `user_id` | Integer | Yes | User to add |

**Returns:** `true` on success

---

#### `createChat`
Create a new chat.

**Access:** User only

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `type` | String | Yes | Chat type: `supergroup`, `channel`, or `group` |
| `title` | String | Yes | Chat title |
| `description` | String | No | Chat description |
| `message_auto_delete_time` | Integer | No | Auto-delete timer in seconds (default 0) |
| `user_ids` | Array of Integer | Group only | User IDs to add (required for `group` type) |

**Returns:** Chat object

---

#### `deleteChatHistory`
Delete the entire chat history.

**Access:** User only

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `chat_id` | Integer/String | Yes | Chat identifier |
| `for_everyone` | Boolean | No | Delete for all members |
| `remove_from_chat_list` | Boolean | No | Also remove from chat list |

**Returns:** `true` on success

---

#### `votePoll`
Vote on a poll.

**Access:** User only

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `chat_id` | Integer/String | Yes | Chat identifier |
| `message_id` | Integer | Yes | Message containing the poll |
| `option_ids` | Array of Integer | Yes | Option IDs to vote for |

**Returns:** `true` on success

---

#### `getCallbackQueryAnswer`
Get the answer to a callback query (simulate pressing an inline button).

**Access:** User only

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `chat_id` | Integer/String | Yes | Chat identifier |
| `message_id` | Integer | Yes | Message identifier |
| `callback_data` | String | Yes | Callback data from the button |

**Returns:** Callback query answer object

---

#### `searchMessages`
Search for messages across all chats.

**Access:** User only

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `query` | String | Yes | Search query |
| `offset` | String | No | Offset for pagination |
| `filter` | String | No | Message type filter (see [filter values](#filter-values)) |
| `min_date` | Integer | No | Min date (unix timestamp) |
| `max_date` | Integer | No | Max date (unix timestamp) |
| `chat_filter` | String | No | Chat type: `private`, `group`, or `channel` |

**Returns:** Array of messages

---

#### `searchChatMessages`
Search for messages within a specific chat.

**Access:** User only

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `chat_id` | Integer/String | Yes | Chat identifier |
| `query` | String | No | Search query |
| `sender_user_id` | Integer | No | Filter by sender |
| `from_message_id` | Integer | No | Start searching from this message ID |
| `filter` | String | No | Message type filter (see [filter values](#filter-values)) |

**Returns:** Array of messages

---

#### `getMessages`
Get multiple messages by their IDs.

**Access:** User only

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `chat_id` | Integer/String | Yes | Chat identifier |
| `message_ids` | Array of Integer | Yes | Message IDs (max 500) |

**Returns:** Array of messages

---

#### `getScheduledMessages`
Get scheduled messages in a chat.

**Access:** User only

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `chat_id` | Integer/String | Yes | Chat identifier |

**Returns:** Array of scheduled messages

---

#### `editMessageScheduling`
Edit the scheduling of a scheduled message.

**Access:** User only

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `chat_id` | Integer/String | Yes | Chat identifier |
| `message_id` | Integer | Yes | Scheduled message ID |
| `send_at` | String/Integer | Yes | New send time: unix timestamp, or `"online"` to send when recipient comes online |
| `repeat_period` | Integer | No | Repeat period in seconds (0–31536000) |

**Returns:** `true` on success

---

#### `getMessagePublicForwards`
Get public forwards of a channel message.

**Access:** User only

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `chat_id` | Integer/String | Yes | Chat identifier |
| `message_id` | Integer | Yes | Message identifier |
| `offset` | String | No | Offset for pagination |
| `limit` | Integer | No | Max results (default 100, max 100) |

**Returns:** Array of messages

---

#### `getMessageStatistics`
Get statistics for a specific message.

**Access:** User only

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `chat_id` | Integer/String | Yes | Chat identifier |
| `message_id` | Integer | Yes | Message identifier |

**Returns:** Message statistics object

---

#### `getChatStatistics`
Get statistics for a chat.

**Access:** User only

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `chat_id` | Integer/String | Yes | Chat identifier |
| `is_dark` | Boolean | No | Use dark theme for graphs |

**Returns:** Chat statistics object

---

#### `getStatisticalGraph`
Get a statistical graph by its token.

**Access:** User only

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `chat_id` | Integer | No | Chat identifier (default 0) |
| `token` | String | Yes | Graph token (from statistics response) |
| `x` | Integer | No | X-axis zoom value (default 0) |

**Returns:** Statistical graph object

---

#### `getChatMessageCalendar`
Get a calendar of message counts by date.

**Access:** User only

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `chat_id` | Integer/String | Yes | Chat identifier |
| `topic` | String | No | Topic filter: `thread`, `forum`, `direct_messages`, or `saved_messages` |
| `topic_id` | Integer | Required with `topic` | Topic ID |
| `filter` | String | No | Message type filter (see [filter values](#filter-values)) |
| `from_message_id` | Integer | No | Start from this message ID |

**Returns:** Message calendar object

---

#### `getChatMessageByDate`
Get the first message on or after a specific date.

**Access:** User only

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `chat_id` | Integer/String | Yes | Chat identifier |
| `date` | Integer | Yes | Unix timestamp |

**Returns:** `Message` object

---

#### `setSupergroupUsername`
Set or remove the username of a supergroup.

**Access:** User only

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `chat_id` | Integer/String | Yes | Supergroup identifier |
| `username` | String | Yes | New username (empty to remove) |

**Returns:** `true` on success

---

#### Chat Invite Link Methods

##### `getChatInviteLink`
Get details of a specific invite link.

**Access:** User only

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `chat_id` | Integer/String | Yes | Chat identifier |
| `invite_link` | String | Yes | The invite link |

**Returns:** Invite link object

##### `getChatInviteLinks`
Get all invite links created by a specific user.

**Access:** User only

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `chat_id` | Integer/String | Yes | Chat identifier |
| `creator_user_id` | Integer | Yes | Creator's user ID |
| `is_revoked` | Boolean | No | Filter revoked links only |
| `offset_date` | Integer | No | Offset date for pagination |
| `offset_invite_link` | String | No | Offset invite link for pagination |
| `limit` | Integer | No | Max results (default 100, max 100) |

**Returns:** Array of invite link objects

##### `getChatInviteLinkCounts`
Get invite link counts by creator.

**Access:** User only

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `chat_id` | Integer/String | Yes | Chat identifier |

**Returns:** Array of invite link count objects

##### `getChatInviteLinkMembers`
Get members who joined via a specific invite link.

**Access:** User only

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `chat_id` | Integer/String | Yes | Chat identifier |
| `invite_link` | String | Yes | The invite link |
| `only_with_expired_subscription` | Boolean | No | Filter expired subscriptions only |
| `offset_date` | Integer | No | Offset date for pagination |
| `limit` | Integer | No | Max results (default 100, max 100) |

**Returns:** Array of chat invite link member objects

##### `getChatInviteLinksFullData`
Get comprehensive invite link data for a chat.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `chat_id` | Integer/String | Yes | Chat identifier |

**Returns:** Full invite link data object

##### `checkChatInviteLink`
Verify an invite link without joining.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `invite_link` | String | Yes | The invite link to check |

**Returns:** Invite link info object

---

<a name="filter-values"></a>
### Filter Values

The `filter` parameter used in search methods accepts the following values:

| Value | Description |
|-------|-------------|
| *(empty)* | No filter (all messages) |
| `animation` | GIF animations |
| `audio` | Audio files |
| `chat_photo` | Chat photo changes |
| `document` | Documents (files) |
| `failed_to_send` | Failed messages |
| `mention` | Messages with mentions |
| `photo` | Photos |
| `photo_and_video` | Photos and videos |
| `pinned` | Pinned messages |
| `unread_mention` | Unread mentions |
| `url` | Messages with URLs |
| `video` | Videos |
| `video_note` | Video notes (round videos) |
| `voice_and_video_note` | Voice and video notes |
| `voice_note` | Voice messages |

---

<a name="modified-features"></a>
## Modified Features

<a name="modified-methods"></a>
### Modified Methods

#### `getChat`
Now also resolves usernames online if they can't be found locally.

#### `deleteMessages`
Extended with range deletion support (see [deleteMessages](#deletemessages) above).

---

<a name="extended-objects"></a>
### Extended Objects

#### `Message`
| New Field | Type | Description |
|-----------|------|-------------|
| `views` | Integer | View count (typically for channel messages) |
| `forwards` | Integer | Forward count |
| `interaction_info` | Object | Detailed interaction data (see below) |

**`interaction_info` fields:**
| Field | Type | Description |
|-------|------|-------------|
| `count.views` | Integer | View count |
| `count.forwards` | Integer | Forward count |
| `count.replies` | Integer | Reply count |
| `count.reactions` | Integer | Reaction count |
| `reactions_info.are_tags` | Boolean | Whether reactions are used as tags |
| `reactions_info.can_get_added_reactions` | Boolean | Whether the reaction list can be fetched |
| `reactions_info.reactions` | Array | Array of reactions (`type`, `total_count`, `is_chosen`) |

#### `ChatMember`
| New Field | Type | Description |
|-----------|------|-------------|
| `joined_date` | Integer | Unix timestamp of when the user joined |
| `inviter` | User | The user who invited this member |

The member list now shows **all bots** in the chat (not just the querying bot).

#### `Chat`
| New Field | Type | Description |
|-----------|------|-------------|
| `is_verified` | Boolean | Whether the chat is verified by Telegram |
| `is_scam` | Boolean | Whether the chat is flagged as scam |
| `is_fake` | Boolean | Whether the chat is flagged as fake |
| `status` | String | Current user's status in the chat (supergroups/channels only): `creator`, `administrator`, `member`, `restricted`, `left`, `kicked` |

#### `User`
| New Field | Type | Description |
|-----------|------|-------------|
| `is_verified` | Boolean | Whether the user is verified by Telegram |
| `is_scam` | Boolean | Whether the user is flagged as scam |
| `is_fake` | Boolean | Whether the user is flagged as fake |
| `user_status` | String | User's online status: `online`, `recently`, `offline`, `week`, `month`, `empty` |
| `last_seen` | Integer | Unix timestamp of last seen time (only when `user_status` is `offline`) |
| `is_deleted` | Boolean | Whether the account is deleted |

**Other changes:**
- The bot now receives updates for all media, even if a self-destruct timer is set.

---

<a name="user-mode"></a>
## User Mode

User mode allows user accounts (not just bots) to access the API. Disabled by default.

**Enable with:** `--allow-users` flag or `TELEGRAM_ALLOW_USERS=1` environment variable.

> **Security:** Never send your 2FA password over plain HTTP. Use HTTPS or run the API locally.

### Authorization Flow

**Step 1.** Request a login code:
```
POST {api_url}/userlogin
```
| Parameter | Type | Description |
|-----------|------|-------------|
| `phone_number` | String | Your Telegram phone number |

Returns your `user_token` as a string.

**Step 2.** Submit the code:
```
POST {api_url}/user{user_token}/authcode
```
| Parameter | Type | Description |
|-----------|------|-------------|
| `code` | Integer | Code sent by Telegram |

**Step 3.** *(Optional)* Submit 2FA password:
```
POST {api_url}/user{user_token}/2fapassword
```
| Parameter | Type | Description |
|-----------|------|-------------|
| `password` | String | 2FA password |

**Step 4.** *(Optional)* Register new account (requires `--allow-users-registration`):
```
POST {api_url}/user{user_token}/registerUser
```
| Parameter | Type | Description |
|-----------|------|-------------|
| `first_name` | String | First name |
| `last_name` | String | *(Optional)* Last name |

### Using User Mode
Replace `/bot{token}/` with `/user{token}/` in all API URLs. You only need to authenticate once — the session persists until you call `logOut`.

Multiple user tokens can be active simultaneously on the same server.

### Unavailable Methods in User Mode
These bot-only methods are not available for user accounts:
`answerCallbackQuery`, `setMyCommands`, `editMessageReplyMarkup`, `uploadStickerFile`, `createNewStickerSet`, `addStickerToSet`, `setStickerPositionInSet`, `deleteStickerFromSet`, `setStickerSetThumb`, `sendInvoice`, `answerShippingQuery`, `answerPreCheckoutQuery`, `setPassportDataErrors`, `sendGame`, `setGameScore`, `getGameHighscores`

User accounts also cannot attach `reply_markup` to messages. Command entities may not be generated in chats without bots.

---

<a name="command-line-parameters"></a>
## Command Line Parameters

### Added Parameters

| Parameter | Description |
|-----------|-------------|
| `--allow-users` | Enable user account authentication |
| `--allow-users-registration` | Enable user account registration |
| `--relative` | Allow only relative file paths in local mode |
| `--insecure` | Allow HTTP connections in non-local mode |
| `--max-batch-operations=<N>` | Max batch operations per call (default 10000) |
| `--stats-hide-sensible-data` | Hide bot token and webhook URL on the stats page |
| `--http-idle-timeout=<seconds>` | HTTP idle timeout (default 500s) |

### Standard Parameters

| Parameter | Description |
|-----------|-------------|
| `--api-id=<ID>` | Telegram API ID *(required)* |
| `--api-hash=<HASH>` | Telegram API hash *(required)* |
| `--local` | Enable local mode (unlimited downloads, 2GB uploads, file URIs, etc.) |
| `--http-port=<PORT>` | HTTP port (default 8081) |
| `-v<N>` / `--verbose=<N>` | Log verbosity: `0` fatal, `1` errors, `2` warnings, `3` info, `4` debug |

---

<a name="docker--environment-variables"></a>
## Docker & Environment Variables

### Quick Start
```bash
docker run -p 8081:8081 \
  --env TELEGRAM_API_ID=YOUR_API_ID \
  --env TELEGRAM_API_HASH=YOUR_API_HASH \
  tdlight/tdlightbotapi
```

### Environment Variables

| Variable | Description |
|----------|-------------|
| `TELEGRAM_API_ID` | *(Required)* Telegram API ID |
| `TELEGRAM_API_HASH` | *(Required)* Telegram API hash |
| `TELEGRAM_WORK_DIR` | Working directory |
| `TELEGRAM_TEMP_DIR` | Temporary directory |
| `TELEGRAM_PORT` | API port (default 8081) |
| `TELEGRAM_STAT` | Enable stats port |
| `TELEGRAM_STAT_HIDE_SENSIBLE_DATA` | Hide sensitive data in stats |
| `TELEGRAM_ALLOW_USERS` | Enable user mode (`1`) |
| `TELEGRAM_ALLOW_USERS_REGISTRATION` | Enable user registration (`1`) |
| `TELEGRAM_LOCAL` | Enable local mode |
| `TELEGRAM_NO_FILE_LIMIT` | Remove file size limits |
| `TELEGRAM_INSECURE` | Allow HTTP in non-local mode |
| `TELEGRAM_RELATIVE` | Use relative file paths |
| `TELEGRAM_MAX_BATCH` | Max batch operations |
| `TELEGRAM_HTTP_IDLE_TIMEOUT` | HTTP idle timeout in seconds |
| `TELEGRAM_MAX_CONNECTIONS` | Max connections |
| `TELEGRAM_MAX_WEBHOOK_CONNECTIONS` | Max webhook connections |
| `TELEGRAM_VERBOSITY` | Log verbosity level |
| `TELEGRAM_PROXY` | Proxy configuration |
| `TELEGRAM_FILTER` | Event filter |
| `TELEGRAM_LOGS` | Log output path |

---

<a name="installation"></a>
## Installation

### Build from Source

```bash
git clone --recursive https://github.com/tdlight-team/tdlight-telegram-bot-api
cd tdlight-telegram-bot-api
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --target install
```

If you forgot `--recursive`:
```bash
git submodule update --init --recursive
```

<a name="dependencies"></a>
## Dependencies

- OpenSSL
- zlib
- C++17 compiler (Clang 5.0+, GCC 7.0+, MSVC 19.1+, Intel C++ 19+)
- gperf *(build only)*
- CMake 3.10+ *(build only)*

<a name="usage"></a>
## Usage

```bash
telegram-bot-api --api-id=YOUR_API_ID --api-hash=YOUR_API_HASH
```

Use `telegram-bot-api --help` for all available options.

In `--local` mode:
- Download files without size limits
- Upload files up to 2000 MB
- Use local file paths and `file://` URIs
- Use HTTP webhooks
- Use any local IP/port for webhooks
- Set `max_webhook_connections` up to 100,000
- Receive absolute local paths in `file_path`

The server accepts HTTP only — use a TLS termination proxy for HTTPS.

Default port: **8081** (change with `--http-port`).

<a name="documentation"></a>
## Documentation

- [Telegram Bots Introduction](https://core.telegram.org/bots)
- [Official Bot API Documentation](https://core.telegram.org/bots/api)
- [Bot API Server Build Instructions](https://tdlib.github.io/telegram-bot-api/build.html)
- [@BotNews](https://t.me/botnews) — Bot API updates
- [@BotTalk](https://t.me/bottalk) — Discussion

<a name="switching"></a>
## Moving a Bot to a Local Server

1. Call [logOut](https://core.telegram.org/bots/api#logout) on `https://api.telegram.org` to deregister
2. Point your bot to your local server address
3. If using `--local` mode, ensure your bot handles absolute file paths from `getFile`

<a name="moving"></a>
## Moving a Bot Between Local Servers

1. Call [deleteWebhook](https://core.telegram.org/bots/api#deletewebhook) on the old server
2. Call [close](https://core.telegram.org/bots/api#close) to close the instance
3. Move the bot's subdirectory (by user ID) from the old server's working directory to the new one
4. Resume requests on the new server

<a name="license"></a>
## License

Licensed under the [Boost Software License](http://www.boost.org/LICENSE_1_0.txt).
