#include "doodle_core/metadata/computer.h"

#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
#include <doodle_lib/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/consign.hpp>
#include <boost/beast/websocket/stream.hpp>

#include "core/app_base.h"
#include "core/http/http_function.h"
#include <memory>

namespace doodle::http {
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_computers, get) {
  person_.check_not_outsourcer();
  auto l_sql       = g_ctx().get<sqlite_database>();
  auto l_computers = l_sql.get_all<computer>();
  co_return in_handle->make_msg(nlohmann::json{} = l_computers);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_computers_instance, get) {
  person_.check_not_outsourcer();
  auto l_sql      = g_ctx().get<sqlite_database>();
  auto l_computer = l_sql.get_by_uuid<computer>(computer_id_);
  co_return in_handle->make_msg(nlohmann::json{} = l_computer);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_computers_instance, delete_) {
  person_.check_not_outsourcer();
  auto l_sql      = g_ctx().get<sqlite_database>();
  auto l_computer = l_sql.get_by_uuid<computer>(computer_id_);
  co_await l_sql.remove<computer>(computer_id_);
  socket_io::broadcast("doodle:computer:delete", nlohmann::json{} = l_computer);
  co_return in_handle->make_msg(nlohmann::json{} = l_computer);
}

class data_computers_socket_io_impl : public std::enable_shared_from_this<data_computers_socket_io_impl> {
  std::shared_ptr<boost::beast::websocket::stream<http::tcp_stream_type>> web_stream_;
  std::shared_ptr<computer> computer_;

  boost::asio::awaitable<void> async_run() {
    co_await init();
    while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
      // boost::beast::flat_buffer l_buffer{};
      std::string l_body{};
      auto l_buffer = boost::asio::dynamic_buffer(l_body);
      if (!web_stream_) co_return;
      co_await web_stream_->async_read(l_buffer);
    }
  }

  // 第一步, 等待计算机发送自身信息, 第二步, 将计算机信息保存到数据库
  boost::asio::awaitable<void> init() {
    boost::beast::flat_buffer l_buffer{};

    co_await web_stream_->async_read(l_buffer);
    auto l_json                     = nlohmann::json::parse(l_buffer.data());
    computer_                       = std::make_shared<computer>(l_json.get<computer>());
    computer_->last_heartbeat_time_ = std::chrono::system_clock::now();
    auto l_sql                      = g_ctx().get<sqlite_database>();
    using namespace sqlite_orm;
    if (l_sql.impl_->storage_any_.count<computer>(where(c(&computer::hardware_id_) == computer_->hardware_id_)) != 0) {
      *computer_ =
          l_sql.impl_->storage_any_.get_all<computer>(where(c(&computer::hardware_id_) == computer_->hardware_id_))
              .front();
      computer_->status_ = computer_status::online;
      computer_->name_   = computer_->name_;
      co_await l_sql.update(computer_);
    } else {
      computer_->status_ = computer_status::online;
      co_await l_sql.install(computer_);
    }
  }

 public:
  explicit data_computers_socket_io_impl(boost::beast::websocket::stream<http::tcp_stream_type> in_stream)
      : web_stream_(std::make_shared<boost::beast::websocket::stream<http::tcp_stream_type>>(std::move(in_stream))) {}

  boost::beast::websocket::stream<http::tcp_stream_type>& get_web_stream() { return *web_stream_; }

  void run() {
    boost::asio::co_spawn(
        g_io_context(), async_run(),
        boost::asio::bind_cancellation_slot(
            app_base::Get().on_cancel.slot(), boost::asio::consign(boost::asio::detached, shared_from_this())
        )
    );
  }
};

void data_computers::websocket_callback(
    boost::beast::websocket::stream<http::tcp_stream_type> in_stream, http::session_data_ptr in_handle
) {
  person_.check_not_outsourcer();
  auto l_impl = std::make_shared<data_computers_socket_io_impl>(std::move(in_stream));
  l_impl->run();
}
bool data_computers::has_websocket() const { return true; }
}  // namespace doodle::http