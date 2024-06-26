#include "dingding_client.h"

#include <doodle_core/core/app_base.h>

namespace doodle::dingding {
void client::begin_refresh_token(chrono::seconds in_seconds) {
  timer_ptr_->expires_after(in_seconds);
  timer_ptr_->async_wait(boost::asio::bind_cancellation_slot(
      app_base::Get().on_cancel.slot(),
      [this](boost::system::error_code ec) {
        if (ec) {
          http_client_core_ptr_->logger()->log(log_loc(), level::warn, "timer_ptr_ error: {}", ec);
          return;
        }
        access_token(app_key, app_secret, true, [this](boost::system::error_code ec, nlohmann::json in_json) {
          if (ec) {
            http_client_core_ptr_->logger()->log(log_loc(), level::err, "refresh_token failed: {}", ec.message());
            return;
          }
          access_token_ = in_json["accessToken"].get<std::string>();
        });
      }
  ));
}
}  // namespace doodle::dingding