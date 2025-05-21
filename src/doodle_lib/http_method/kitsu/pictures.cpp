//
// Created by TD on 25-5-12.
//
#include "doodle_core/metadata/preview_file.h"
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_front_end.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> pictures_thumbnails_organisations_get::callback(
    session_data_ptr in_handle
) {
  FSys::path l_filename = in_handle->capture_->get("id");
  auto l_path = g_ctx().get<kitsu_ctx_t>().root_ / "pictures" / "thumbnails" / FSys::split_uuid_path(l_filename);
  auto l_ext  = l_filename.extension();
  co_return in_handle->make_msg(l_path.replace_extension(), kitsu::mime_type(l_ext));
}
boost::asio::awaitable<boost::beast::http::message_generator> pictures_thumbnails_square_preview_files_get::callback(
    session_data_ptr in_handle
) {
  FSys::path l_filename = in_handle->capture_->get("id");
  auto l_path           = g_ctx().get<kitsu_ctx_t>().root_ / "pictures" / "thumbnails" / "squ" / "are" /
                (std::string{"square-"} + l_filename.stem().generic_string());
  auto l_ext = l_filename.extension();
  co_return in_handle->make_msg(l_path, kitsu::mime_type(l_ext));
}

boost::asio::awaitable<boost::beast::http::message_generator> pictures_thumbnails_preview_files_get::callback(
    session_data_ptr in_handle
) {
  FSys::path l_filename = in_handle->capture_->get("id");
  auto l_path = g_ctx().get<kitsu_ctx_t>().root_ / "pictures" / "thumbnails" / FSys::split_uuid_path(l_filename);
  auto l_ext  = l_filename.extension();
  co_return in_handle->make_msg(l_path.replace_extension(), kitsu::mime_type(l_ext));
}
boost::asio::awaitable<boost::beast::http::message_generator> pictures_thumbnails_persons_get::callback(
    session_data_ptr in_handle
) {
  FSys::path l_filename = in_handle->capture_->get("id");
  auto l_path = g_ctx().get<kitsu_ctx_t>().root_ / "pictures" / "thumbnails" / FSys::split_uuid_path(l_filename);
  auto l_ext  = l_filename.extension();
  co_return in_handle->make_msg(l_path.replace_extension(), kitsu::mime_type(l_ext));
}

boost::asio::awaitable<boost::beast::http::message_generator> pictures_originals_preview_files_download_get::callback(
    session_data_ptr in_handle
) {
  auto l_sql            = g_ctx().get<sqlite_database>();
  FSys::path l_filename = in_handle->capture_->get("id");
  auto l_pre_file       = l_sql.get_by_uuid<preview_file>(in_handle->capture_->get_uuid());
  if (l_pre_file.extension_ == ".png") {
    auto l_path = g_ctx().get<kitsu_ctx_t>().root_ / "pictures" / "original" / FSys::split_uuid_path(l_filename);
    co_return in_handle->make_msg(l_path, kitsu::mime_type(l_pre_file.extension_));
  } else if (l_pre_file.extension_ == ".pdf") {
    auto l_path = g_ctx().get<kitsu_ctx_t>().root_ / "pictures" / "previews" / FSys::split_uuid_path(l_filename);
    co_return in_handle->make_msg(l_path, kitsu::mime_type(l_pre_file.extension_));
  } else if (l_pre_file.extension_ == ".mp4") {
    auto l_path = g_ctx().get<kitsu_ctx_t>().root_ / "pictures" / "original" / FSys::split_uuid_path(l_filename);
    co_return in_handle->make_msg(l_path, ".png");
  } else {
    auto l_path = g_ctx().get<kitsu_ctx_t>().root_ / "pictures" / "previews" / FSys::split_uuid_path(l_filename);
    co_return in_handle->make_msg(l_path, kitsu::mime_type(l_pre_file.extension_));
  }
  throw_exception(http_request_error{boost::beast::http::status::not_found, "file not found"});
}

}  // namespace doodle::http
