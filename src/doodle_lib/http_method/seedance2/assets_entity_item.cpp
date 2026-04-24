#include "doodle_core/exception/exception.h"
#include <doodle_core/metadata/kitsu_ctx_t.h>
#include <doodle_core/metadata/seedance2/assets_entity.h>
#include <doodle_core/metadata/seedance2/assets_entity_item.h>
#include <doodle_core/metadata/seedance2/group.h>
#include <doodle_core/metadata/seedance2/task.h>

#include "doodle_lib/core/http/http_function.h"
#include <doodle_lib/core/global_function.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include "reg.h"
#include <filesystem>
#include <fmt/format.h>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <opencv2/opencv.hpp>
#include <sqlite_orm/sqlite_orm.h>

namespace doodle::http::seedance2 {
namespace sd2 = doodle::seedance2;

namespace {
bool is_image_extension(const FSys::path& in_ext) {
  static const std::set<FSys::path> l_image_exts{".png", ".jpg", ".jpeg"};
  return l_image_exts.contains(in_ext);
}
/// 视频扩展名
bool is_video_extension(const FSys::path& in_ext) {
  static const std::set<FSys::path> l_video_exts{".mp4", ".avi", ".m4v", ".mov", ".webm"};
  return l_video_exts.contains(in_ext);
}
}  // namespace

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_asset_library_entity_item, post) {
  auto l_file = in_handle->get_file();
  DOODLE_CHICK_HTTP(!l_file.empty() && FSys::exists(l_file), bad_request, "无文件");

  auto l_entity_item        = std::make_shared<sd2::assets_entity_item>();
  l_entity_item->parent_id_ = parent_id_;
  auto l_sql                = get_sqlite_database();
  co_await l_sql.install(l_entity_item);

  if (auto l_ext = l_file.extension(); is_image_extension(l_ext)) {
    auto l_file_pictures =
        g_ctx().get<kitsu_ctx_t>().get_sd2_asset_library_entity_pictures_item_file(l_entity_item->uuid_id_, ".png");
    auto l_file_thumbnail =
        g_ctx().get<kitsu_ctx_t>().get_sd2_asset_library_entity_thumbnail_item_file(l_entity_item->uuid_id_, ".png");
    if (auto l_p = l_file_pictures.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
    if (auto l_p = l_file_thumbnail.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
    {
      auto l_image  = cv::imread(l_file.generic_string());
      auto l_resize = std::min(192.0 / l_image.cols, 108.0 / l_image.rows);
      cv::resize(l_image, l_image, cv::Size(100, 100));
      cv::imwrite(l_file_thumbnail.generic_string(), l_image);
    }
    if (l_file.extension() != ".png") {
      auto l_image = cv::imread(l_file.generic_string());
      if (l_image.empty()) throw_exception(doodle_error{"无法读取图片文件"});
      cv::imwrite(l_file_pictures.generic_string(), l_image);
    } else
      FSys::rename(l_file, l_file_pictures);
  } else if (is_video_extension(l_file.extension())) {
    // 大于 100M 直接拒绝 上传
    DOODLE_CHICK_HTTP(FSys::file_size(l_file) < 100 * 1024 * 1024, bad_request, "视频文件大小超过100M，无法上传");

    auto l_file_pictures =
        g_ctx().get<kitsu_ctx_t>().get_sd2_asset_library_entity_pictures_item_file(l_entity_item->uuid_id_, ".mp4");
    auto l_file_thumbnail =
        g_ctx().get<kitsu_ctx_t>().get_sd2_asset_library_entity_thumbnail_item_file(l_entity_item->uuid_id_, ".png");
    if (auto l_p = l_file_pictures.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
    if (auto l_p = l_file_thumbnail.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
    {
      // 生成预览文件
      auto l_video = cv::VideoCapture{l_file.generic_string()};
      // 读取第一帧生成预览文件
      cv::Mat l_image{};
      l_video >> l_image;
      if (l_image.empty()) throw_exception(doodle_error{"视频解码失败"});
      auto l_resize = std::min(192.0 / l_image.cols, 108.0 / l_image.rows);
      cv::resize(l_image, l_image, cv::Size(100, 100));

      if (auto l_p = l_file_thumbnail.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
      cv::imwrite(l_file_thumbnail.generic_string(), l_image);
    }
    if (l_file.extension() != ".mp4") {
      auto l_video = cv::VideoCapture{l_file.generic_string()};
      if (!l_video.isOpened()) throw_exception(doodle_error{"无法读取视频文件"});
      cv::VideoWriter l_writer{
          l_file_pictures.generic_string(), cv::VideoWriter::fourcc('a', 'v', 'c', '1'), l_video.get(cv::CAP_PROP_FPS),
          cv::Size{
              boost::numeric_cast<int>(l_video.get(cv::CAP_PROP_FRAME_WIDTH)),
              boost::numeric_cast<int>(l_video.get(cv::CAP_PROP_FRAME_HEIGHT))
          }
      };
      if (!l_writer.isOpened()) throw_exception(doodle_error{"无法写入视频文件"});
      cv::Mat l_frame;
      while (l_video.read(l_frame)) l_writer.write(l_frame);
    } else
      FSys::rename(l_file, l_file_pictures);
  } else {
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "不支持的文件类型"});
  }

  co_return in_handle->make_msg_204();
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_asset_library_entity_item_instance, delete_) {
  co_return in_handle->make_msg_204();
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_asset_library_entity_pictures_item, get) {
  auto l_file = g_ctx().get<kitsu_ctx_t>().get_sd2_asset_library_entity_pictures_item_file(id_, ".png");
  DOODLE_CHICK_HTTP(FSys::exists(l_file), not_found, "文件不存在");
  co_return in_handle->make_msg(l_file, kitsu::mime_type(l_file.extension()));
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_asset_library_entity_thumbnail_item, get) {
  auto l_file = g_ctx().get<kitsu_ctx_t>().get_sd2_asset_library_entity_thumbnail_item_file(id_, ".png");
  DOODLE_CHICK_HTTP(FSys::exists(l_file), not_found, "文件不存在");
  co_return in_handle->make_msg(l_file, kitsu::mime_type(l_file.extension()));
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_animation_waiting, get) {
  auto l_file = g_ctx().get<kitsu_ctx_t>().front_end_root_ / "seedance2" / "animation" / "waiting.mp4";
  DOODLE_CHICK_HTTP(FSys::exists(l_file), not_found, "文件不存在");
  co_return in_handle->make_msg(l_file, kitsu::mime_type(l_file.extension()));
}
}  // namespace doodle::http::seedance2