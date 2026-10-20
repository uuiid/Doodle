#include "ai_light_render.h"

#include <doodle_core/exception/exception.h>
#include <doodle_core/metadata/move_create.h>

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/file_sys.h>
#include <doodle_lib/exe_warp/import_and_render_ue.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/logger/logger.h>
#include <doodle_lib/long_task/image_to_move.h>

#include <boost/asio/use_awaitable.hpp>

#include <algorithm>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "ue_exe.h"

namespace doodle::http {
namespace {

ai_light::ai_light_task_payload_t make_ue_payload(
    const ai_light::ai_light_task_payload_t& in_payload, const FSys::path& in_local_root
) {
  ai_light::ai_light_task_payload_t l_out = in_payload;
  const FSys::path l_out_dir              = in_local_root / in_payload.out_dir_;
  l_out.out_dir_                          = l_out_dir;
  for (auto& l_c : l_out.candidates_) {
    l_c.frames_dir_ = l_out_dir / l_c.frames_dir_;
    l_c.png_path_   = l_out_dir / l_c.png_path_;
    l_c.exr_path_   = l_out_dir / l_c.exr_path_;
    l_c.movie_path_ = l_out_dir / l_c.movie_path_;
  }
  return l_out;
}

std::vector<FSys::path> collect_render_frames(const FSys::path& in_frames_dir) {
  std::vector<FSys::path> l_frames{};
  std::error_code l_ec{};
  for (const auto& l_entry : FSys::directory_iterator{in_frames_dir, l_ec}) {
    if (!l_entry.is_regular_file(l_ec)) continue;
    if (l_entry.path().extension() != ".png") continue;
    l_frames.emplace_back(l_entry.path());
  }
  std::sort(l_frames.begin(), l_frames.end());
  return l_frames;
}

FSys::path sync_assembly_files(
    const ai_light::ai_light_task_payload_t& in_payload, std::shared_ptr<spdlog::logger> in_logger
) {
  DOODLE_CHICK(
      !in_payload.project_dir_.empty(),
      "AI 灯光第 {} 轮的载荷里没有装配工程相对目录（project_dir），无法确定本机工程缓存位置",
      in_payload.iteration_
  );
  DOODLE_CHICK(
      !in_payload.assembly_files_.empty(),
      "AI 灯光第 {} 轮的载荷里没有装配同步清单（assembly_files），无法把工程同步到本机",
      in_payload.iteration_
  );

  const FSys::path l_work_root = core_set::get_set().user_work_root_;
  if (l_work_root.empty())
    throw_exception(
        doodle_error{"工作机没有配置 UE 工程缓存根（user_work_root），无法把装配文件同步到本地，本轮无法渲染"}
    );
  const FSys::path l_prj_dir = l_work_root / in_payload.project_dir_;

  if (!FSys::exists(l_prj_dir)) FSys::create_directories(l_prj_dir);

  SPDLOG_LOGGER_INFO(
      in_logger, "AI 灯光第 {} 轮：同步 {} 个装配文件到本地工程 {}", in_payload.iteration_,
      in_payload.assembly_files_.size(), l_prj_dir.generic_string()
  );
  for (auto&& l_file : in_payload.assembly_files_) {
    DOODLE_CHICK(!l_file.from_.empty(), "装配同步清单里有一条空的源路径，无法同步");
    FSys::copy_diff(l_file.from_, l_prj_dir / l_file.to_, in_logger);
  }

  const auto l_uproject = l_prj_dir / FSys::path{in_payload.ue_project_path_}.filename();
  if (!FSys::exists(l_uproject))
    throw_exception(doodle_error{"装配同步后仍找不到 UE 工程文件 {}，无法启动 UE", l_uproject.generic_string()});
  return l_uproject;
}

}  // namespace

boost::asio::awaitable<void> ai_light_round_distributed::run() {
  auto l_logger       = create_logger();
  auto l_kitsu_client = create_kitsu_client();
  l_kitsu_client->set_logger(l_logger);

  ai_light::round_render_report_t l_report{};
  std::string l_error_msg{};
  FSys::path l_local_out_dir{};

  try {
    payload_ = task_info_.command_.get<ai_light::ai_light_task_payload_t>();

    if (payload_.ue_project_path_.empty()) throw_exception(doodle_error{"载荷里没有 .uproject 路径，无法启动 UE"});
    if (payload_.render_map_.empty()) throw_exception(doodle_error{"载荷里没有渲染关卡，无法渲染"});
    if (payload_.level_sequence_.empty()) throw_exception(doodle_error{"载荷里没有 level sequence，无法渲染视频"});
    if (payload_.candidates_.empty()) throw_exception(doodle_error{"载荷里没有候选，没有可渲染的东西"});
    if (payload_.movie_pipeline_config_.empty())
      SPDLOG_LOGGER_WARN(l_logger, "AI 灯光第 {} 轮：载荷里没有电影管线配置，UE 会按内置参数另建一份", payload_.iteration_);

    const FSys::path l_local_root =
        core_set::get_set().get_cache_root(FSys::path{ai_light::g_ai_light_local_cache_flag});
    const auto l_ue_payload = make_ue_payload(payload_, l_local_root);
    l_local_out_dir         = l_ue_payload.out_dir_;

    SPDLOG_LOGGER_INFO(
        l_logger, "AI 灯光会话 {} 第 {} 轮：{} 个候选，本地输出目录 {}", payload_.session_id_, payload_.iteration_,
        payload_.candidates_.size(), l_local_out_dir.generic_string()
    );

    if (!FSys::exists(l_local_out_dir)) FSys::create_directories(l_local_out_dir);
    const FSys::path l_report_path = l_local_out_dir / ai_light::g_ai_light_render_report_file_name;
    if (FSys::exists(l_report_path)) FSys::remove(l_report_path);

    const FSys::path l_uproject = sync_assembly_files(payload_, l_logger);
    import_and_render_ue_ns::fix_project(l_uproject);
    import_and_render_ue_ns::fix_config(l_uproject);

    const nlohmann::json l_payload_json = l_ue_payload;
    auto l_tmp_path = FSys::write_tmp_file("ai_light", l_payload_json.dump(), ".json");
    const auto l_ue_arg = std::vector<std::string>{
        l_uproject.generic_string(), payload_.render_map_, "-log", "-AllowStdOutLogVerbosity", "-ForceLogFlush",
        "-Unattended", fmt::format("-DoodleAiLightParams={}", l_tmp_path.generic_string())};

    co_await async_run_ue(
        l_ue_arg, l_logger, l_kitsu_client, true, std::make_shared<server_task_info::run_time_info_t>()
    );

    if (!FSys::exists(l_report_path))
      throw_exception(
          doodle_error{
              "UE 没有写出渲染报告 {}（渲染进程失败，或 -DoodleAiLightParams 不被识别）", l_report_path.generic_string()
          }
      );

    std::ifstream l_report_if{l_report_path};
    l_report = nlohmann::json::parse(l_report_if).get<ai_light::round_render_report_t>();
    SPDLOG_LOGGER_INFO(
        l_logger, "AI 灯光第 {} 轮：读到渲染报告，{} 个候选，帧率 {}", payload_.iteration_, l_report.candidates_.size(),
        l_report.frame_rate_
    );

    const std::int32_t l_fps = l_report.frame_rate_ > 0 ? l_report.frame_rate_ : 25;

    for (auto& l_cr : l_report.candidates_) {
      if (l_cr.status_ != ai_light::candidate_render_status::rendered) continue;

      const uuid l_id = l_cr.candidate_id_;
      if (l_id.is_nil()) {
        SPDLOG_LOGGER_WARN(l_logger, "AI 灯光第 {} 轮：报告里有一个候选没有 id，无法上传其产物，按失败处理", payload_.iteration_);
        l_cr.status_ = ai_light::candidate_render_status::failed;
        continue;
      }

      const auto l_payload_cand = std::find_if(
          l_ue_payload.candidates_.begin(), l_ue_payload.candidates_.end(),
          [&l_id](const ai_light::ai_light_task_candidate_t& in_c) { return in_c.candidate_id_ == l_id; }
      );
      if (l_payload_cand == l_ue_payload.candidates_.end()) {
        SPDLOG_LOGGER_WARN(
            l_logger, "AI 灯光第 {} 轮：报告里的候选 {} 不在载荷里，找不到它的本地产物，按失败处理",
            payload_.iteration_, l_id
        );
        l_cr.status_ = ai_light::candidate_render_status::failed;
        continue;
      }
      const FSys::path l_frames_dir = l_payload_cand->frames_dir_;
      const FSys::path l_png        = l_payload_cand->png_path_;
      const FSys::path l_movie      = l_payload_cand->movie_path_;

      const auto l_frames = collect_render_frames(l_frames_dir);
      l_cr.frame_count_   = static_cast<std::int32_t>(l_frames.size());
      if (l_frames.size() < 2) {
        SPDLOG_LOGGER_WARN(
            l_logger, "AI 灯光第 {} 轮：候选 {} 报告为渲染成功，但帧目录 {} 里只有 {} 张帧，合成不出视频，按失败处理",
            payload_.iteration_, l_id, l_frames_dir.generic_string(), l_frames.size()
        );
        l_cr.status_ = ai_light::candidate_render_status::failed;
        continue;
      }

      try {
        std::error_code l_ec{};
        FSys::copy_file(l_frames.front(), l_png, FSys::copy_options::overwrite_existing, l_ec);
        if (l_ec) throw_exception(doodle_error{"首帧海报写出失败 {}：{}", l_png.generic_string(), l_ec.message()});

        ::doodle::detail::create_move(
            l_movie, l_logger, movie::image_attr::make_default_attr(l_frames), image_size{}, l_fps
        );
      } catch (const std::exception& l_ex) {
        SPDLOG_LOGGER_ERROR(
            l_logger, "AI 灯光第 {} 轮：候选 {} 的产物合成失败，按失败处理：{}", payload_.iteration_, l_id, l_ex.what()
        );
        l_cr.status_ = ai_light::candidate_render_status::failed;
        continue;
      }

      try {
        co_await l_kitsu_client->begin_ai_light_candidate_sequence(l_id);
        for (const auto& l_frame : l_frames)
          co_await l_kitsu_client->upload_ai_light_candidate_frame(l_id, l_frame);
        co_await l_kitsu_client->finish_ai_light_candidate_sequence(l_id, l_fps);
      } catch (const std::exception& l_ex) {
        SPDLOG_LOGGER_ERROR(
            l_logger, "AI 灯光第 {} 轮：候选 {} 的产物上传失败，按失败处理：{}", payload_.iteration_, l_id, l_ex.what()
        );
        l_cr.status_ = ai_light::candidate_render_status::failed;
        continue;
      }

      SPDLOG_LOGGER_INFO(
          l_logger, "AI 灯光第 {} 轮：候选 {} 完成，{} 帧 @ {} fps，海报 {}，视频 {}", payload_.iteration_, l_id,
          l_cr.frame_count_, l_fps, l_png.generic_string(), l_movie.generic_string()
      );
    }
  } catch (const std::exception& l_ex) {
    SPDLOG_LOGGER_ERROR(l_logger, "AI 灯光渲染任务异常: {}", l_ex.what());
    l_error_msg = l_ex.what();
    if (l_report.error_.empty()) l_report.error_ = l_error_msg;
    if (!l_local_out_dir.empty())
      SPDLOG_LOGGER_INFO(l_logger, "AI 灯光本地产物保留在 {}", l_local_out_dir.generic_string());
  }

  try {
    const nlohmann::json l_report_json = l_report;
    co_await l_kitsu_client->post_ai_light_render_report(task_info_.uuid_id_, l_report_json);
  } catch (const std::exception& l_ex) {
    SPDLOG_LOGGER_ERROR(l_logger, "回传渲染报告失败: {}", l_ex.what());
    if (l_error_msg.empty()) l_error_msg = l_ex.what();
  }

  if (!l_local_out_dir.empty())
    SPDLOG_LOGGER_INFO(
        l_logger, "AI 灯光第 {} 轮结束，本地产物目录（保留不删）：{}", payload_.iteration_, l_local_out_dir.generic_string()
    );

  task_info_.status_   = l_error_msg.empty() ? server_task_info_status::completed : server_task_info_status::failed;
  task_info_.end_time_ = std::chrono::system_clock::now();
  try {
    co_await l_kitsu_client->put_job_info(
        task_info_.uuid_id_, nlohmann::json{{"status", task_info_.status_}, {"end_time", task_info_.end_time_}}
    );
  } catch (const std::exception& l_ex) {
    SPDLOG_LOGGER_ERROR(l_logger, "更新任务信息失败: {}", l_ex.what());
  }
  co_return;
}

}  // namespace doodle::http
