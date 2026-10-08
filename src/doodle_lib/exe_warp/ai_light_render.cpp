//
// AI 灯光分布式任务：一轮候选的渲染（工作机侧，方案 §15.12）
//

#include "ai_light_render.h"

#include <doodle_core/exception/exception.h>

#include <doodle_lib/core/file_sys.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/logger/logger.h>

#include <boost/asio/use_awaitable.hpp>

#include <fstream>
#include <string>

#include "ue_exe.h"

namespace doodle::http {

boost::asio::awaitable<void> ai_light_round_distributed::run() {
  auto l_logger       = create_logger();
  auto l_kitsu_client = create_kitsu_client();
  l_kitsu_client->set_logger(l_logger);

  ai_light::round_render_report_t l_report{};
  std::string l_error_msg{};

  try {
    payload_ = task_info_.command_.get<ai_light::ai_light_task_payload_t>();
    SPDLOG_LOGGER_INFO(
        l_logger, "AI 灯光会话 {} 第 {} 轮：{} 个候选，输出目录 {}", payload_.session_id_, payload_.iteration_,
        payload_.candidates_.size(), payload_.out_dir_.generic_string()
    );

    // 三样硬前置。缺任何一个 UE 都起不来 —— 直接当整轮失败回传，
    // 别让服务端那条会话在 generating 里干等一个永远不会来的报告。
    if (payload_.baseline_level_.empty())
      throw_exception(doodle_error{"载荷里没有基线关卡（服务端推导失败），无法渲染"});
    if (payload_.ue_project_path_.empty()) throw_exception(doodle_error{"载荷里没有 .uproject 路径，无法启动 UE"});
    if (payload_.candidates_.empty()) throw_exception(doodle_error{"载荷里没有候选，没有可渲染的东西"});

    if (!FSys::exists(payload_.out_dir_)) FSys::create_directories(payload_.out_dir_);
    // 上一次的残留报告必须先删：否则 commandlet 崩在写报告之前，我们会把上一轮的结果
    // 当成本轮的回传（候选 uuid 对不上，服务端会判「候选不属于本轮」而整轮 400）。
    const FSys::path l_report_path = payload_.out_dir_ / ai_light::g_ai_light_render_report_file_name;
    if (FSys::exists(l_report_path)) FSys::remove(l_report_path);

    auto l_tmp_path = FSys::write_tmp_file("ai_light", nlohmann::json{payload_}.dump(), ".json");

    // create_lock = true：UE 是整台机器的独占资源，async_run_ue 内部会拿全局队列锁。
    // `-AllowCommandletRendering` 不能漏：`CommandletHelpers::TickEngine` 只在这个开关出现时才 tick
    // 渲染（`Commandlet.cpp:161`），否则 commandlet 抓到的是一张没渲染过的空画面。
    co_await async_run_ue(
        {payload_.ue_project_path_, "-windowed", "-log", "-AllowStdOutLogVerbosity", "-ForceLogFlush", "-Unattended",
         "-AllowCommandletRendering", "-run=DoodleLighting", fmt::format("-Params={}", l_tmp_path.generic_string())},
        l_logger, l_kitsu_client, true, std::make_shared<server_task_info::run_time_info_t>()
    );

    if (!FSys::exists(l_report_path))
      throw_exception(
          doodle_error{"UE 没有写出渲染报告 {}（commandlet 失败或 -Params 不被识别）", l_report_path.generic_string()}
      );

    std::ifstream l_report_if{l_report_path};
    l_report = nlohmann::json::parse(l_report_if).get<ai_light::round_render_report_t>();
    SPDLOG_LOGGER_INFO(l_logger, "AI 灯光第 {} 轮：读到渲染报告，{} 个候选", payload_.iteration_, l_report.candidates_.size());
  } catch (const std::exception& l_ex) {
    SPDLOG_LOGGER_ERROR(l_logger, "AI 灯光渲染任务异常: {}", l_ex.what());
    l_error_msg = l_ex.what();
    // 异常也要回传一份「整轮失败」的报告：服务端靠 error_ 给会话写 failure_reason_，
    // 没有候选就代表一张都没渲染成功（apply_render_report 的两条出口之一）。
    if (l_report.error_.empty()) l_report.error_ = l_error_msg;
  }

  try {
    co_await l_kitsu_client->post_ai_light_render_report(task_info_.uuid_id_, nlohmann::json{l_report});
  } catch (const std::exception& l_ex) {
    SPDLOG_LOGGER_ERROR(l_logger, "回传渲染报告失败: {}", l_ex.what());
    if (l_error_msg.empty()) l_error_msg = l_ex.what();
  }

  // 分布式契约：run() 返回前必须落到终态，否则服务端库里这条任务会一直是 running，
  // 而 run_next_task 会因为它一直不派新任务给这台机器（方案 §10.4）。
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
