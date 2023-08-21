//
// Created by td_main on 2023/8/21.
//

#include "work.h"

namespace doodle {
namespace render_farm {
void work::run() {
  auto l_s = boost::asio::make_strand(g_io_context());

  if (!ptr_->resolver_) {
    ptr_->resolver_ = std::make_shared<resolver>(l_s);
  }
  if (!ptr_->socket_) {
    ptr_->socket_ = std::make_shared<socket>(l_s);
  }
  if (!ptr_->timer_) {
    ptr_->timer_ = std::make_shared<timer>(l_s);
  }

  ptr_->timer_->expires_after(2s);
  ptr_->timer_->async_wait([this](boost::system::error_code ec) {
    if (ec) {
      DOODLE_LOG_INFO("{}", ec.message());
      return;
    }
    run();
  });
  if (!ptr_->socket_->is_open()) {
    ptr_->resolver_->async_resolve(
        ptr_->server_ip, "50021",
        [this](boost::system::error_code ec, boost::asio::ip::tcp::resolver::results_type results) {
          if (ec) {
            DOODLE_LOG_INFO("{}", ec.message());
            return;
          }
          boost::asio::async_connect(
              *ptr_->socket_, results,
              [this](boost::system::error_code ec, boost::asio::ip::tcp::endpoint endpoint) {
                if (ec) {
                  DOODLE_LOG_INFO("{}", ec.message());
                  return;
                }
                DOODLE_LOG_INFO("连接成功服务器");
                do_register();
              }
          );
        }
    );
  } else {
    DOODLE_LOG_INFO("socket is open");
    do_register();
  }
}
void work::do_register() {}
}  // namespace render_farm
}  // namespace doodle