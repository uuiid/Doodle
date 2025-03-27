//
// Created by TD on 25-3-6.
//

#include "../base/config.h"

#include <../../core/http/http_function.h>
namespace doodle::http {
namespace {
boost::asio::awaitable<boost::beast::http::message_generator> config(session_data_ptr in_handle) {
  co_return in_handle->make_msg(R"({
    "is_self_hosted": true,
    "crisp_token": "",
    "dark_theme_by_default": null,
    "indexer_configured": true,
    "saml_enabled": false,
    "saml_idp_name": "",
    "default_locale": "zh",
    "default_timezone": "Asia/Shanghai"
})");
}
}  // namespace
void register_config(http_route& in_r) {
  in_r.reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/config", config));
}
}  // namespace doodle::http