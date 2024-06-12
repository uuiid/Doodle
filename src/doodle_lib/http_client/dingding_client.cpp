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
        access_token(app_key, app_secret, true);
      }
  ));
}
void client::access_token(const std::string& in_app_key, const std::string& in_app_secret, bool in_auto_expire) {
  async_access_token(
      in_app_key, in_app_secret, in_auto_expire,
      [this](boost::system::error_code ec, nlohmann::json in_json) {
        if (ec) {
          http_client_core_ptr_->logger()->log(log_loc(), level::err, "access_token failed: {}", ec.message());
          return;
        }
      }
  );
}

}  // namespace doodle::dingding