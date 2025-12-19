
#pragma once
#include <doodle_core/core/global_function.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/doodle_core_pch.h>

#include <filesystem>
#include <string>

namespace doodle::http {

struct kitsu_ctx_t {
  std::string url_;
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

  FSys::path get_tiles_file_path(const uuid& in_uuid) {
    return root_ / "pictures" / "tiles" / FSys::split_uuid_path(fmt::format("{}{}", in_uuid, ".png"));
  }

  FSys::path get_movie_source_file(const uuid& in_uuid, const std::string& in_ext = {}) {
    return get_source_file(in_uuid, false, in_ext);
  }
  FSys::path get_movie_preview_file(const uuid& in_uuid, const std::string& in_ext = {}) {
    return get_preview_file_path(in_uuid, false, in_ext);
  }
  FSys::path get_movie_lowdef_file(const uuid& in_uuid, const std::string& in_ext = {}) {
    return get_lowdef_file_path(in_uuid, false, in_ext);
  }
  FSys::path get_picture_original_file(const uuid& in_uuid, const std::string& in_ext = {}) {
    return get_source_file(in_uuid, true, in_ext);
  }
  FSys::path get_picture_preview_file(const uuid& in_uuid, const std::string& in_ext = {}) {
    return get_preview_file_path(in_uuid, true, in_ext);
  }

 private:
  FSys::path get_source_file(const uuid& in_uuid, bool is_image, const std::string& in_ext = {}) {
    if (is_image)
      return root_ / "pictures" / "original" /
             FSys::split_uuid_path(fmt::format("{}{}", in_uuid, fix_ext(is_image, in_ext)));
    else
      return root_ / "movies" / "source" /
             FSys::split_uuid_path(fmt::format("{}{}", in_uuid, fix_ext(is_image, in_ext)));
  }

  FSys::path get_preview_file_path(const uuid& in_uuid, bool is_image, const std::string& in_ext = {}) {
    if (is_image)
      return root_ / "pictures" / "previews" /
             FSys::split_uuid_path(fmt::format("{}{}", in_uuid, fix_ext(is_image, in_ext)));
    else
      return root_ / "movies" / "previews" /
             FSys::split_uuid_path(fmt::format("{}{}", in_uuid, fix_ext(is_image, in_ext)));
  }
  FSys::path get_lowdef_file_path(const uuid& in_uuid, bool is_image, const std::string& in_ext = {}) {
    if (is_image)
      return root_ / "pictures" / "lowdef" /
             FSys::split_uuid_path(fmt::format("{}{}", in_uuid, fix_ext(is_image, in_ext)));
    else
      return root_ / "movies" / "lowdef" /
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