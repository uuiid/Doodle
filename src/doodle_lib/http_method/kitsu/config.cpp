//
// Created by TD on 25-3-6.
//

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

#include "kitsu.h"
namespace doodle::http {

boost::asio::awaitable<boost::beast::http::message_generator> config_get::callback_arg(session_data_ptr in_handle) {
  co_return in_handle->make_msg(std::string{R"({
    "is_self_hosted": true,
    "crisp_token": "",
    "dark_theme_by_default": null,
    "indexer_configured": true,
    "saml_enabled": false,
    "saml_idp_name": "",
    "default_locale": "zh",
    "default_timezone": "Asia/Shanghai"
})"});
}
boost::asio::awaitable<boost::beast::http::message_generator> deepseek_key_get::callback_arg(session_data_ptr in_handle) {
  auto l_list = g_ctx().get<kitsu_ctx_t>().deepseek_keys_;
  co_return in_handle->make_msg((nlohmann::json{} = l_list).dump());
}

}  // namespace doodle::http