//
// Created by TD on 2026-11-10.
//

#include "face_capture_transcode.h"

#include <doodle_core/exception/exception.h>

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/face_capture/capture_session.h>
#include <doodle_lib/core/face_capture/face_capture_types.h>
#include <doodle_lib/core/face_capture/png_sequence_writer.h>
#include <doodle_lib/core/file_sys.h>
#include <doodle_lib/exe_warp/ue_exe.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <string>
#include <vector>

namespace doodle {

namespace {

constexpr std::string_view k_curves_file_name{"face_curves.dhfc"};

constexpr std::string_view k_log_file_name{"face_capture_log.json"};

FSys::path session_dir_of(const uuid& in_session_id) {
  return face_capture::capture_session::default_root() / fmt::format("{}", in_session_id);
}

std::vector<FSys::path> list_png(const FSys::path& in_dir) {
  std::vector<FSys::path> l_files{};
  if (!FSys::exists(in_dir)) return l_files;
  for (const auto& l_entry : FSys::directory_iterator{in_dir}) {
    if (!l_entry.is_regular_file()) continue;
    if (l_entry.path().extension() != ".png") continue;
    l_files.emplace_back(l_entry.path());
  }
  std::sort(l_files.begin(), l_files.end());
  return l_files;
}

bool is_digit(char in_char) { return std::isdigit(static_cast<unsigned char>(in_char)) != 0; }

std::string digits_of(const FSys::path& in_path) {
  auto l_stem = in_path.stem().generic_string();
  auto l_end  = l_stem.find_last_of("0123456789");
  if (l_end == std::string::npos) return {};
  auto l_begin = l_end;
  while (l_begin > 0 && is_digit(l_stem[l_begin - 1])) --l_begin;
  return l_stem.substr(l_begin, l_end - l_begin + 1);
}

void normalize_frames(const FSys::path& in_frames_dir) {
  auto l_files = list_png(in_frames_dir);
  DOODLE_CHICK(!l_files.empty(), "帧目录 {} 中没有 png 文件", in_frames_dir.generic_string());

  bool l_need_rename  = false;
  std::string l_first = digits_of(l_files.front());
  if (l_first.empty() || l_first.size() != static_cast<std::size_t>(face_capture::k_png_digits)) {
    l_need_rename = true;
  }

  if (!l_need_rename) {
    std::int64_t l_expect = std::stoll(l_first);
    for (const auto& l_path : l_files) {
      auto l_digits = digits_of(l_path);
      if (l_digits.size() != l_first.size() || std::stoll(l_digits) != l_expect) {
        l_need_rename = true;
        break;
      }
      ++l_expect;
    }
  }
  if (!l_need_rename) return;

  auto l_tmp_dir = in_frames_dir.parent_path() / fmt::format("{}_rename", in_frames_dir.filename().generic_string());

  if (FSys::exists(l_tmp_dir)) FSys::remove_all(l_tmp_dir);
  FSys::create_directories(l_tmp_dir);

  for (std::size_t l_index = 0; l_index < l_files.size(); ++l_index) {
    const auto l_name = fmt::format("frame_{:0{}d}.png", l_index, face_capture::k_png_digits);
    FSys::rename(l_files[l_index], l_tmp_dir / l_name);
  }

  FSys::remove_all(in_frames_dir);
  FSys::rename(l_tmp_dir, in_frames_dir);
}

std::string read_log_code(const FSys::path& in_out_dir) {
  auto l_path = in_out_dir / std::string{k_log_file_name};
  if (!FSys::exists(l_path)) return {};

  try {
    auto const l_json = nlohmann::json::parse(FSys::ifstream{l_path});
    if (!l_json.is_object()) return {};

    const auto l_code   = l_json.contains("code") ? l_json.at("code").get<std::string>() : std::string{};
    const auto l_stage  = l_json.contains("stage") ? l_json.at("stage").get<std::string>() : std::string{};
    const auto l_detail = l_json.contains("detail") ? l_json.at("detail").get<std::string>() : std::string{};
    if (l_code.empty()) return {};
    return fmt::format("stage={} code={} detail={}", l_stage, l_code, l_detail);
  } catch (...) {
    return {};
  }
}

}  // namespace

void from_json(const nlohmann::json& in_json, face_capture_transcode_arg& out_obj) {
  in_json.at("preview_id").get_to(out_obj.preview_id_);
}

void to_json(nlohmann::json& in_json, const face_capture_transcode_arg& out_obj) {
  in_json["preview_id"] = out_obj.preview_id_;
}

boost::asio::awaitable<void> face_capture_transcode_arg::run() {
  const auto l_id_str = fmt::format("{}", preview_id_);
  DOODLE_CHICK(!preview_id_.is_nil(), "面部捕捉转码任务缺少 preview_id, 无法确定对应会话");

  const auto l_session_dir = session_dir_of(preview_id_);
  const auto l_video       = l_session_dir / "take.mp4";
  const auto l_frames_dir  = l_session_dir / "frames";
  const auto l_curves_dir  = l_session_dir / "curves";

  DOODLE_CHICK(FSys::exists(l_video), "会话 {} 缺少录制视频 {}", l_id_str, l_video.generic_string());

  if (FSys::exists(l_frames_dir)) FSys::remove_all(l_frames_dir);
  if (FSys::exists(l_curves_dir)) FSys::remove_all(l_curves_dir);

  SPDLOG_LOGGER_INFO(logger_ptr_, "开始把 {} 转码为 png 序列", l_video.generic_string());
  const auto l_png_result = face_capture::write_png_sequence(
      face_capture::png_sequence_option{
          .video_            = l_video,
          .session_dir_      = l_session_dir,
          .decode_max_width_ = face_capture::k_default_width,
      }
  );
  SPDLOG_LOGGER_INFO(
      logger_ptr_, "转码完成 {} 帧 {}x{}, 输出目录 {}", l_png_result.frame_count_, l_png_result.width_,
      l_png_result.height_, l_frames_dir.generic_string()
  );

  DOODLE_CHICK(
      l_png_result.frame_count_ >= 2, "会话 {} 只解码出 {} 帧, 无法解算动捕", l_id_str, l_png_result.frame_count_
  );

  normalize_frames(l_frames_dir);

  FSys::create_directories(l_curves_dir);

  const auto l_uproject = ue_exe_ns::find_ue_project_file(core_set::get_set().ue4_path);
  DOODLE_CHICK(
      !l_uproject.empty(), "UE 路径 {} 下找不到 .uproject 文件", core_set::get_set().ue4_path.generic_string()
  );

  const auto l_ue_arg = std::vector<std::string>{
      l_uproject.generic_string(),
      "-run=MetaHumanFace",
      fmt::format(R"(-FramesDir="{}")", l_frames_dir.generic_string()),
      fmt::format(R"(-OutDir="{}")", l_curves_dir.generic_string()),
      fmt::format("-FPS={}/{}", face_capture::k_fps, 1),
      "-log",
      "-AllowStdOutLogVerbosity",
      "-ForceLogFlush",
      "-Unattended",
  };

  SPDLOG_LOGGER_INFO(logger_ptr_, "开始运行 UE 解算面部动捕 {}", l_ue_arg);
  try {
    co_await async_run_ue(l_ue_arg, logger_ptr_, kitsu_client_, true);
  } catch (const doodle_error& in_err) {
    const auto l_log_code = read_log_code(l_curves_dir);
    if (l_log_code.empty()) throw;
    throw_exception(doodle_error{"UE 面部动捕解算失败: {}; 原因: {}", in_err.what(), l_log_code});
  }

  const auto l_curves = l_curves_dir / std::string{k_curves_file_name};
  DOODLE_CHICK(
      FSys::exists(l_curves), "UE 解算完成但未生成动捕文件 {}, 日志: {}", l_curves.generic_string(),
      read_log_code(l_curves_dir)
  );

  SPDLOG_LOGGER_INFO(logger_ptr_, "面部动捕解算完成 {}", l_curves.generic_string());
  co_return;
}

}  // namespace doodle
