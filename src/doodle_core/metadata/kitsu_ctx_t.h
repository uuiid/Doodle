
#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/doodle_core_pch.h>

#include <doodle_lib/core/file_sys.h>
#include <doodle_lib/core/global_function.h>

#include <doodle_core/metadata/seedance2/canvas_media.h>

#include <filesystem>
#include <fmt/format.h>
#include <string>

namespace doodle::http {

struct kitsu_ctx_t {
  std::string access_token_;
  /// 产生的资产储存位置
  std::filesystem::path root_;
  /// 前端部署的问价所在位置
  std::filesystem::path front_end_root_;
  /// deepseek ai 的key
  std::vector<std::string> deepseek_keys_;

  /// 即梦授权
  std::string ji_meng_access_key_id_;
  std::string ji_meng_secret_access_key_;

  /// 会话 jwt token
  std::string secret_;
  /// 服务器 协议和域名(基本在发送电子邮件时使用)
  std::string domain_protocol_;
  std::string domain_name_;
  // 获取制作规范 md文件
  FSys::path get_production_specifications_file() { return root_ / "production_specifications.md"; }
  FSys::path get_ue_plugins_version_file() { return root_ / "ue_plugins_version.txt"; }
  FSys::path get_ue_plugins_file(const std::int32_t in_major, const std::int32_t in_minor, const std::int32_t in_patch) {
    return root_ / "ue_plugins" / fmt::format("UE_{}.{}.{}.zip", in_major, in_minor, in_patch);
  }

  // seedance2_asset_library_entity_pictures_item
  FSys::path get_sd2_asset_library_entity_pictures_item_file(const uuid& in_uuid, const std::string& in_ext = {}) {
    return root_ / "sd2" / "pictures" / "asset_library" /
           FSys::split_uuid_path(fmt::format("{}{}", in_uuid, fix_ext(true, in_ext)));
  }
  // seedance2_asset_library_entity_thumbnail_item
  FSys::path get_sd2_asset_library_entity_thumbnail_item_file(const uuid& in_uuid, const std::string& in_ext = {}) {
    return root_ / "sd2" / "thumbnails" / "asset_library" /
           FSys::split_uuid_path(fmt::format("{}{}", in_uuid, fix_ext(true, in_ext)));
  }

  // seedance2_thumbnail
  FSys::path get_sd2_thumbnail_file(const uuid& in_uuid) {
    return root_ / "sd2" / "thumbnails" / FSys::split_uuid_path(fmt::format("{}.png", in_uuid));
  }
  // seedance2_pictures
  FSys::path get_sd2_pictures_file(const uuid& in_uuid, const std::string& in_ext = {}) {
    return root_ / "sd2" / "pictures" /
           FSys::split_uuid_path(fmt::format("{}{}", in_uuid, fix_ext(true, in_ext)));
  }
  // seedance2_canvas_media — 按 media_role 路由到不同子目录，缩略图固定 .png
  FSys::path get_sd2_canvas_media_file(const uuid& in_uuid, seedance2::media_role in_role, const std::string& in_ext = {}) {
    auto subdir = [&]() -> FSys::path {
      switch (in_role) {
        case seedance2::media_role::thumbnail: return "thumbnails";
        case seedance2::media_role::preview:   return "previews";
        default:                               return {};
      }
    }();
    auto path = FSys::path{"sd2"} / "canvas_media";
    if (!subdir.empty()) path /= subdir;
    auto ext = in_role == seedance2::media_role::thumbnail ? ".png" : fix_ext(true, in_ext);
    return root_ / path / FSys::split_uuid_path(fmt::format("{}{}", in_uuid, ext));
  }

  FSys::path get_jobs_logs_file(const uuid& in_uuid) {
    return root_ / "jobs" / FSys::split_uuid_path(fmt::format("{}.log", in_uuid));
  }

  FSys::path get_inference_materials_video(const uuid& in_uuid) {
    return root_ / "inference_materials" / "video" / FSys::split_uuid_path(fmt::format("{}.mp4", in_uuid));
  }
  FSys::path get_inference_materials_image(const uuid& in_uuid) {
    return root_ / "inference_materials" / "image" / FSys::split_uuid_path(fmt::format("{}.png", in_uuid));
  }

  FSys::path get_tiles_file_path(const uuid& in_uuid) { return root_ / get_tiles_file_path_(in_uuid); }
  FSys::path get_pictures_thumbnails_file(const uuid& in_uuid, const std::string& in_ext = {}) {
    return root_ / get_pictures_thumbnails_file_(in_uuid, in_ext);
  }
  FSys::path get_pictures_thumbnails_square_file(const uuid& in_uuid, const std::string& in_ext = {}) {
    return root_ / get_pictures_thumbnails_square_file_(in_uuid, in_ext);
  }

  FSys::path get_movie_source_file(const uuid& in_uuid, const std::string& in_ext = {}) {
    return root_ / get_source_file(in_uuid, false, in_ext);
  }
  FSys::path get_movie_preview_file(const uuid& in_uuid, const std::string& in_ext = {}) {
    return root_ / get_preview_file_path(in_uuid, false, in_ext);
  }
  FSys::path get_movie_lowdef_file(const uuid& in_uuid, const std::string& in_ext = {}) {
    return root_ / get_lowdef_file_path(in_uuid, false, in_ext);
  }
  FSys::path get_pictures_original_file(const uuid& in_uuid, const std::string& in_ext = {}) {
    return root_ / get_source_file(in_uuid, true, in_ext);
  }
  FSys::path get_pictures_preview_file(const uuid& in_uuid, const std::string& in_ext = {}) {
    return root_ / get_preview_file_path(in_uuid, true, in_ext);
  }
  FSys::path get_attachment_file(const uuid& in_uuid) { return root_ / get_attachment_file_(in_uuid); }

  // 外包获取

  FSys::path get_outsource_pictures_original_file(const uuid& in_uuid, const std::string& in_ext = {}) {
    return root_ / "outsource" / get_source_file(in_uuid, true, in_ext);
  }
  FSys::path get_outsource_pictures_preview_file(const uuid& in_uuid, const std::string& in_ext = {}) {
    return root_ / "outsource" / get_preview_file_path(in_uuid, true, in_ext);
  }

 private:
  FSys::path get_attachment_file_(const uuid& in_uuid) {
    return FSys::path{"files"} / "attachments" / FSys::split_uuid_path(fmt::to_string(in_uuid));
  }
  FSys::path get_tiles_file_path_(const uuid& in_uuid) {
    return FSys::path{"pictures"} / "tiles" / FSys::split_uuid_path(fmt::format("{}{}", in_uuid, ".png"));
  }
  FSys::path get_pictures_thumbnails_file_(const uuid& in_uuid, const std::string& in_ext = {}) {
    return FSys::path{"pictures"} / "thumbnails" /
           FSys::split_uuid_path(fmt::format("{}{}", in_uuid, fix_ext(true, in_ext)));
  }
  FSys::path get_pictures_thumbnails_square_file_(const uuid& in_uuid, const std::string& in_ext = {}) {
    return FSys::path{"pictures"} / "thumbnails_square" /
           FSys::split_uuid_path(fmt::format("{}{}", in_uuid, fix_ext(true, in_ext)));
  }
  FSys::path get_source_file(const uuid& in_uuid, bool is_image, const std::string& in_ext = {}) {
    if (is_image)
      return FSys::path{"pictures"} / "original" /
             FSys::split_uuid_path(fmt::format("{}{}", in_uuid, fix_ext(is_image, in_ext)));
    else
      return FSys::path{"movies"} / "source" /
             FSys::split_uuid_path(fmt::format("{}{}", in_uuid, fix_ext(is_image, in_ext)));
  }

  FSys::path get_preview_file_path(const uuid& in_uuid, bool is_image, const std::string& in_ext = {}) {
    if (is_image)
      return FSys::path{"pictures"} / "previews" /
             FSys::split_uuid_path(fmt::format("{}{}", in_uuid, fix_ext(is_image, in_ext)));
    else
      return FSys::path{"movies"} / "previews" /
             FSys::split_uuid_path(fmt::format("{}{}", in_uuid, fix_ext(is_image, in_ext)));
  }
  FSys::path get_lowdef_file_path(const uuid& in_uuid, bool is_image, const std::string& in_ext = {}) {
    if (is_image)
      return FSys::path{"pictures"} / "lowdef" /
             FSys::split_uuid_path(fmt::format("{}{}", in_uuid, fix_ext(is_image, in_ext)));
    else
      return FSys::path{"movies"} / "lowdef" /
             FSys::split_uuid_path(fmt::format("{}{}", in_uuid, fix_ext(is_image, in_ext)));
  }
  std::string fix_ext(bool is_image, const std::string& in_ext) {
    if (!in_ext.empty()) return in_ext.front() != '.' ? fmt::format(".{}", in_ext) : in_ext;

    if (is_image)
      return ".png";
    else
      return ".mp4";

    return in_ext;
  }
};

}  // namespace doodle::http