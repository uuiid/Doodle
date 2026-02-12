//
// Created by TD on 25-5-12.
//
#include "doodle_core/exception/exception.h"
#include "doodle_core/metadata/organisation.h"
#include "doodle_core/metadata/preview_file.h"
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_front_end.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

#include "core/http/http_function.h"
#include <opencv2/opencv.hpp>
namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> pictures_thumbnails_organisations_png::get(
    session_data_ptr in_handle
) {
  auto l_path = g_ctx().get<kitsu_ctx_t>().get_pictures_thumbnails_file(id_);
  auto l_ext  = l_path.extension();
  DOODLE_CHICK(FSys::exists(l_path), "组织缩略图不存在 组织 id {}", id_);
  co_return in_handle->make_msg(l_path, kitsu::mime_type(l_ext));
}

/// 处理组织缩略图
void handle_organisation_thumbnail(const FSys::path& in_file, const FSys::path& in_save_path) {
  auto l_image = cv::imread(in_file.string());

  static const cv::Size2d l_size{400, 400};
  cv::resize(l_image, l_image, l_size);
  if (auto l_p = in_save_path.parent_path(); exists(l_p)) FSys::create_directories(l_p);
  if (exists(in_save_path)) remove(in_save_path);
  cv::imwrite(in_save_path.string(), l_image);
}

boost::asio::awaitable<boost::beast::http::message_generator> pictures_thumbnails_organisations::post(
    session_data_ptr in_handle
) {
  person_.check_admin();
  auto l_org = g_ctx().get<sqlite_database>().get_by_uuid<organisation>(id_);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始设置组织 {} 缩略图", person_.person_.email_,
      person_.person_.get_full_name(), id_
  );

  FSys::path l_file{};
  if (auto l_fs = in_handle->get_files(); l_fs.empty())
    co_return in_handle->make_msg(nlohmann::json{});
  else
    l_file = l_fs.front();
  FSys::path l_save_path = g_ctx().get<kitsu_ctx_t>().get_pictures_thumbnails_file(id_);
  handle_organisation_thumbnail(l_file, l_save_path);
  socket_io::broadcast("organisation:set-thumbnail", nlohmann::json{{"organisation_id", id_}});

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成设置组织 {} 缩略图 file {}", person_.person_.email_,
      person_.person_.get_full_name(), id_, l_file.filename().generic_string()
  );

  co_return in_handle->make_msg(
      nlohmann::json{{"thumbnail_path", fmt::format("pictures/thumbnails/organisations/{}.png", id_)}}
  );
}
boost::asio::awaitable<boost::beast::http::message_generator> pictures_thumbnails_square_preview_files::get(
    session_data_ptr in_handle
) {
  FSys::path l_filename = fmt::format("{}.png", id_);
  auto l_path           = g_ctx().get<kitsu_ctx_t>().get_pictures_thumbnails_square_file(id_);
  auto l_ext = l_filename.extension();
  DOODLE_CHICK(FSys::exists(l_path), "缩略图不存在 文件 {}", l_path.generic_string());
  co_return in_handle->make_msg(l_path, kitsu::mime_type(l_ext));
}

boost::asio::awaitable<boost::beast::http::message_generator> pictures_thumbnails_preview_files::get(
    session_data_ptr in_handle
) {
  auto l_path = g_ctx().get<kitsu_ctx_t>().get_pictures_thumbnails_file(id_);
  auto l_ext  = l_path.extension();
  DOODLE_CHICK(FSys::exists(l_path), "缩略图不存在 文件 {}", l_path.generic_string());
  co_return in_handle->make_msg(l_path, kitsu::mime_type(l_ext));
}
boost::asio::awaitable<boost::beast::http::message_generator> pictures_thumbnails_persons::get(
    session_data_ptr in_handle
) {
  auto l_path = g_ctx().get<kitsu_ctx_t>().get_pictures_thumbnails_file(id_);
  auto l_ext  = l_path.extension();
  DOODLE_CHICK(FSys::exists(l_path), "缩略图不存在 文件 {}", l_path.generic_string());
  co_return in_handle->make_msg(l_path, kitsu::mime_type(l_ext));
}

boost::asio::awaitable<boost::beast::http::message_generator> pictures_originals_preview_files_download::get(
    session_data_ptr in_handle
) {
  auto l_sql        = g_ctx().get<sqlite_database>();
  auto l_pre_file   = l_sql.get_by_uuid<preview_file>(id_);
  FSys::path l_path = person_.is_outsourcer() ? g_ctx().get<kitsu_ctx_t>().get_outsource_pictures_original_file(id_)
                                              : g_ctx().get<kitsu_ctx_t>().get_pictures_original_file(id_);
  DOODLE_CHICK(FSys::exists(l_path), "原始图片不存在 文件 {}", l_path.generic_string());
  co_return in_handle->make_msg(l_path, kitsu::mime_type(l_pre_file.extension_));
}
boost::asio::awaitable<boost::beast::http::message_generator> pictures_previews_preview_files::get(
    session_data_ptr in_handle
) {
  auto l_path = person_.is_outsourcer() ? g_ctx().get<kitsu_ctx_t>().get_outsource_pictures_preview_file(id_)
                                        : g_ctx().get<kitsu_ctx_t>().get_pictures_preview_file(id_);
  auto l_ext  = l_path.extension();
  DOODLE_CHICK(FSys::exists(l_path), "缩略图不存在 文件 {}", l_path.generic_string());
  co_return in_handle->make_msg(l_path, kitsu::mime_type(l_ext));
}
boost::asio::awaitable<boost::beast::http::message_generator> pictures_originals_preview_files::get(
    session_data_ptr in_handle
) {
  auto l_path = person_.is_outsourcer() ? g_ctx().get<kitsu_ctx_t>().get_outsource_pictures_original_file(id_)
                                        : g_ctx().get<kitsu_ctx_t>().get_pictures_original_file(id_);
  auto l_ext  = l_path.extension();
  DOODLE_CHICK(FSys::exists(l_path), "原始图片不存在 文件 {}", l_path.generic_string());
  co_return in_handle->make_msg(l_path, kitsu::mime_type(l_ext));
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(movies_originals_preview_files_download, get) {
  DOODLE_CHICK_HTTP(!person_.is_outsourcer(), unauthorized, "无权限下载");

  auto l_path     = g_ctx().get<kitsu_ctx_t>().get_movie_preview_file(preview_file_id_);
  auto l_pre_file = g_ctx().get<sqlite_database>().get_by_uuid<preview_file>(preview_file_id_);
  auto l_ext      = l_path.extension();
  co_return in_handle->make_msg(
      l_path, http_header_ctrl{
                  .mine_type_     = kitsu::mime_type(l_ext),
                  .is_attachment_ = true,
                  .attachment_filename_ =
                      fmt::format("movie_preview_{}.{}", l_pre_file.original_name_, l_ext.string().substr(1))
              }
  );
}
}  // namespace doodle::http
