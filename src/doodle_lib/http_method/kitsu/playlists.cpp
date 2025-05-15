//
// Created by TD on 25-5-14.
//
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> playlists_entities_preview_files_get::callback(
    session_data_ptr in_handle
) {


  co_return in_handle->make_msg(nlohmann::json{});
}

}  // namespace doodle::http