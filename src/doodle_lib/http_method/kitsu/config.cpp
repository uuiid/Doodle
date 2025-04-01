//
// Created by TD on 25-3-6.
//

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

#include "kitsu.h"
namespace doodle::http {

boost::asio::awaitable<boost::beast::http::message_generator> config_get::callback(session_data_ptr in_handle) {
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

}  // namespace doodle::http