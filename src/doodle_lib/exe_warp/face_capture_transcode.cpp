//
// Created by TD on 2026-11-10.
//

#include "face_capture_transcode.h"

#include <doodle_core/exception/exception.h>

#include <doodle_lib/core/face_capture/capture_session.h>
#include <doodle_lib/core/face_capture/face_capture_types.h>
#include <doodle_lib/core/file_sys.h>
#include <doodle_lib/exe_warp/ue_exe.h>

#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>

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

constexpr std::int32_t k_png_compression{3};

FSys::path session_dir_of(const uuid& in_session_id) {
  return face_capture::capture_session::default_root() / fmt::format("{}", in_session_id);
}

bool write_png_frame(const FSys::path& in_dir, std::int32_t in_index, const cv::Mat& in_image) {
  auto l_name = fmt::format("frame_{:0{}d}.png", in_index, face_capture::k_png_digits);
  return cv::imwrite((in_dir / l_name).string(), in_image, {cv::IMWRITE_PNG_COMPRESSION, k_png_compression});
}

std::int32_t decode_to_png(const FSys::path& in_video, const FSys::path& in_frames_dir) {
  cv::VideoCapture l_capture{in_video.string()};
  DOODLE_CHICK(l_capture.isOpened(), "无法打开视频文件 {}", in_video.generic_string());

  std::int32_t l_count = 0;
  cv::Mat l_frame{};
  while (l_capture.read(l_frame)) {
    if (l_frame.empty()) continue;
    DOODLE_CHICK(
        write_png_frame(in_frames_dir, l_count, l_frame), "写出第 {} 帧 png 失败, 目录 {}",
        l_count, in_frames_dir.generic_string()
    );
    ++l_count;
  }
  l_capture.release();
  return l_count;
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
  in_json.at("ue_project").get_to(out_obj.ue_project_);
}

void to_json(nlohmann::json& in_json, const face_capture_transcode_arg& out_obj) {
  in_json["preview_id"]  = out_obj.preview_id_;
  in_json["ue_project"]  = out_obj.ue_project_;
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
  FSys::create_directories(l_frames_dir);

  SPDLOG_LOGGER_INFO(logger_ptr_, "开始把 {} 转码为 png 序列", l_video.generic_string());
  const auto l_frame_count = decode_to_png(l_video, l_frames_dir);
  SPDLOG_LOGGER_INFO(
      logger_ptr_, "转码完成 {} 帧, 输出目录 {}", l_frame_count, l_frames_dir.generic_string()
  );

  DOODLE_CHICK(l_frame_count >= 2, "会话 {} 只解码出 {} 帧, 无法解算动捕", l_id_str, l_frame_count);

  FSys::create_directories(l_curves_dir);

  DOODLE_CHICK(!ue_project_.empty(), "面部捕捉转码任务缺少 ue_project, 无法运行 UE 解算");

  const auto l_ue_arg = std::vector<std::string>{
      ue_project_.generic_string(),
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
