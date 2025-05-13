//
// Created by TD on 25-5-12.
//
#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_front_end.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> pictures_thumbnails_organisations_get::callback(
    session_data_ptr in_handle
) {
  auto l_filename = in_handle->capture_->get("id");
  auto l_path     = g_ctx().get<kitsu_ctx_t>().root_ / "pictures" / "thumbnails" / FSys::split_uuid_path(l_filename);
  auto l_ext      = l_path.extension();
  co_return in_handle->make_msg(l_path.replace_extension(), kitsu::mime_type(l_ext));
}

}  // namespace doodle::http