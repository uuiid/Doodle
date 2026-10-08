
#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/doodle_core_pch.h>
#include <doodle_core/metadata/seedance2/canvas_media.h>

#include <doodle_lib/core/file_sys.h>
#include <doodle_lib/core/global_function.h>

#include <filesystem>
#include <fmt/format.h>
#include <string>

namespace doodle::http {

struct kitsu_ctx_t {
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
  /// 深度估计模型路径（相对于 root_ 或绝对路径）
  std::filesystem::path depth_model_path_{"models/depth/da3mono_large.onnx"};

  FSys::path get_depth_model_path() {
    if (depth_model_path_.is_absolute()) return depth_model_path_;
    return root_ / depth_model_path_;
  }
  // 获取制作规范 md文件
  FSys::path get_production_specifications_file() { return root_ / "production_specifications.md"; }
  FSys::path get_ue_plugins_version_file() { return root_ / "ue_plugins_version.txt"; }
  FSys::path get_ue_plugins_file(
      const std::int32_t in_major, const std::int32_t in_minor, const std::int32_t in_patch
  ) {
    return root_ / "ue_plugins" / fmt::format("UE_{}.{}.{}.zip", in_major, in_minor, in_patch);
  }

  // seedance2_thumbnail
  FSys::path get_sd2_thumbnail_file(const uuid& in_uuid) {
    return root_ / "sd2" / "thumbnails" / FSys::split_uuid_path(fmt::format("{}.png", in_uuid));
  }
  // seedance2_pictures
  FSys::path get_sd2_pictures_file(const uuid& in_uuid, const std::string& in_ext = {}) {
    return root_ / "sd2" / "pictures" / FSys::split_uuid_path(fmt::format("{}{}", in_uuid, fix_ext(true, in_ext)));
  }
  // seedance2_canvas_media — 按 media_role 路由到不同子目录，缩略图固定 .png
  // 注意: 这里的 media_role 必须写全 `doodle::seedance2::`。本结构体位于 namespace doodle::http 内,
  // 一旦 `doodle::http::seedance2` 这个名字空间先被声明(例如先 include 了 http_client 下的头),
  // 裸写 `seedance2::media_role` 就会优先绑定到 doodle::http::seedance2 而编译失败。
  FSys::path get_sd2_canvas_media_file(
      const uuid& in_uuid, doodle::seedance2::media_role in_role, const std::string& in_ext = {}
  ) {
    auto subdir = [&]() -> FSys::path {
      switch (in_role) {
        case doodle::seedance2::media_role::thumbnail:
          return "thumbnails";
        case doodle::seedance2::media_role::preview:
          return "previews";
        default:
          return {};
      }
    }();
    auto path = FSys::path{"sd2"} / "canvas_media";
    if (!subdir.empty()) path /= subdir;
    auto ext = in_role == doodle::seedance2::media_role::thumbnail ? ".png" : fix_ext(true, in_ext);
    return root_ / path / FSys::split_uuid_path(fmt::format("{}{}", in_uuid, ext));
  }

  FSys::path get_jobs_logs_file(const uuid& in_uuid) {
    return root_ / "jobs" / FSys::split_uuid_path(fmt::format("{}.log", in_uuid));
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

  // 获取时间
  FSys::path get_time() { return root_ / "time"; }
  // 获取版本
  FSys::path get_version() { return root_ / "version.txt"; }
  // seedance2 waiting
  FSys::path get_seedance2_waiting() { return root_ / "seedance2" / "animation" / "waiting.mp4"; }

  // ---------------------------------------------------------------------------
  // AI 灯光（docs/ue_ai_lighting_plan.md）
  //
  // 会话产物是**由 uuid 标识的生成物**，与 sd2 / pictures / jobs 同性质，所以挂在资产根
  // root_ 下、和它们同一套约定，而不是挂在项目目录下：
  //
  //   <root>/ai_light/<s[0:3]>/<s[3:6]>/<会话 uuid>/ref/<参考图 uuid>.png
  //   <root>/ai_light/<s[0:3]>/<s[3:6]>/<会话 uuid>/<轮次 uuid>/<候选 uuid>.png
  //   <root>/ai_light/<s[0:3]>/<s[3:6]>/<会话 uuid>/<轮次 uuid>/<候选 uuid>.exr
  //
  // 四个要点：
  //   * **文件名就是各自的 uuid**，不用序号（序号落盘会撞车：同一目录下两张图都叫 ref_00）。
  //     uuid 必须提前分配好，不能等 ORM 在 insert 时回填 —— 路径就是它的函数。
  //   * **轮次目录也用轮次 uuid，不用 it_001 序号**（批 7）：候选行自带 session_id_ 与
  //     iteration_id_，于是「候选 → 产物路径」是一次纯计算，不必回查轮次行拿序号；
  //     序号那套会让路径依赖「另一张表里那个可变字段」。
  //   * **只有会话目录分桶**（split_uuid_path，与 sd2/jobs 一致）。分桶是为了不让单个目录
  //     随会话数线性膨胀；而一个会话的参考图是几十张、单轮候选固定 6 张，内层再分桶只有噪声。
  //   * 因为只依赖 root_ 与会话 uuid，**不再依赖 episode / shot / 项目路径** ——
  //     少掉「算不出产物目录就没法上传参考图」这一整类失败。
  //
  // 参考图与中间候选渲染都**不注册 preview_file**，由专用接口从这些路径直接提供；
  // 只有定稿候选才注册（source = auto_light_generate），免得镜头预览列表被 6×N 张候选淹没。
  // ---------------------------------------------------------------------------

  /// AI 灯光会话目录
  ///
  /// 这一组 AI 灯光 getter 都是 `const` 的：它们只按 uuid 拼路径，不改任何状态。
  /// 于是「候选 → 产物路径」可以做成吃 `const kitsu_ctx_t&` 的纯函数
  /// （`doodle::ai_light::candidate_render_path`，批 7 / D44）。
  FSys::path get_ai_light_session_dir(const uuid& in_session_id) const {
    return root_ / "ai_light" / FSys::split_uuid_path(fmt::to_string(in_session_id));
  }
  /// 会话的参考图目录（在会话目录**之内** —— 不是兄弟目录，否则同镜头的多个会话会共用一处）
  FSys::path get_ai_light_reference_dir(const uuid& in_session_id) const {
    return get_ai_light_session_dir(in_session_id) / "ref";
  }
  /// 单张参考图：文件名就是**它自己的** uuid
  FSys::path get_ai_light_reference_file(const uuid& in_session_id, const uuid& in_reference_id) const {
    return get_ai_light_reference_dir(in_session_id) / fmt::format("{}.png", in_reference_id);
  }
  /// 某一轮的候选渲染目录。目录名就是**轮次自己的 uuid**（见上面第二个要点）。
  FSys::path get_ai_light_iteration_dir(const uuid& in_session_id, const uuid& in_iteration_id) const {
    return get_ai_light_session_dir(in_session_id) / fmt::to_string(in_iteration_id);
  }
  /// 单个候选的渲染图：文件名就是候选自己的 uuid。
  /// in_linear = true 取线性 EXR（不下发浏览器），false 取显示空间 PNG。
  FSys::path get_ai_light_candidate_file(
      const uuid& in_session_id, const uuid& in_iteration_id, const uuid& in_candidate_id, bool in_linear = false
  ) const {
    return get_ai_light_iteration_dir(in_session_id, in_iteration_id) /
           fmt::format("{}{}", in_candidate_id, in_linear ? ".exr" : ".png");
  }
  /// 删会话时的备份目标。
  ///
  /// 与 up_file_base（`root_path_ / "backup" / <小时> / …`）同构：备份挂在**自己这一片**根下，
  /// 于是源与目标必然同卷、FSys::rename 不会因为「root_ 在本地盘而项目在 NAS」失败。
  /// 时间戳由调用方给，本方法不读当前时间 —— 路径 getter 里藏一个时间依赖会让它没法测。
  FSys::path get_ai_light_backup_dir(const uuid& in_session_id, const std::string& in_time_str) const {
    return root_ / "ai_light" / "backup" / in_time_str / fmt::to_string(in_session_id);
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