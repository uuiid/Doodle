//
// AI 灯光分布式任务：一轮候选的渲染（工作机侧）
//

#include "ai_light_render.h"

#include <doodle_core/exception/exception.h>

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/file_sys.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/logger/logger.h>

#include <boost/asio/use_awaitable.hpp>

#include <fstream>
#include <memory>
#include <string>

#include "ue_exe.h"

namespace doodle::http {
namespace {

/// 把载荷里那些**相对路径**拼到工作机本地根上，返回一份给 UE 用的副本（D38/D39）。
///
/// 为什么不就地改 payload_：payload_ 是服务端那份「相对命名」契约的本地镜像，
/// 报告之外还要拿它反查候选（候选 id → 本地产物名）。就地改成绝对路径并不会更省事，
/// 却会让「工作机本地盘长什么样」粘在这份结构上 —— 将来谁把它写进日志或回传，
/// 就把工作机的本地路径漏给服务端了，而服务端既用不到也不该知道（D38）。
ai_light::ai_light_task_payload_t make_ue_payload(
    const ai_light::ai_light_task_payload_t& in_payload, const FSys::path& in_local_root
) {
  ai_light::ai_light_task_payload_t l_out = in_payload;
  const FSys::path l_out_dir              = in_local_root / in_payload.out_dir_;
  l_out.out_dir_                          = l_out_dir;
  for (auto& l_c : l_out.candidates_) {
    // png_path_ / exr_path_ 相对的是 out_dir_，不是本地根，所以和 l_out_dir 拼。
    l_c.png_path_ = l_out_dir / l_c.png_path_;
    l_c.exr_path_ = l_out_dir / l_c.exr_path_;
  }
  return l_out;
}

}  // namespace

boost::asio::awaitable<void> ai_light_round_distributed::run() {
  auto l_logger       = create_logger();
  auto l_kitsu_client = create_kitsu_client();
  l_kitsu_client->set_logger(l_logger);

  ai_light::round_render_report_t l_report{};
  std::string l_error_msg{};
  // 本轮的本地输出目录。声明在 try 外面是为了在异常路径的日志里也能报出来 ——
  // 本地产物不删（D43），那就得让日志说清去哪儿捡。
  FSys::path l_local_out_dir{};

  try {
    payload_ = task_info_.command_.get<ai_light::ai_light_task_payload_t>();

    // 三样硬前置。缺任何一个 UE 都起不来 —— 直接当整轮失败回传，
    // 别让服务端那条会话在 generating 里干等一个永远不会来的报告。
    if (payload_.baseline_level_.empty())
      throw_exception(doodle_error{"载荷里没有基线关卡（服务端推导失败），无法渲染"});
    if (payload_.ue_project_path_.empty()) throw_exception(doodle_error{"载荷里没有 .uproject 路径，无法启动 UE"});
    if (payload_.candidates_.empty()) throw_exception(doodle_error{"载荷里没有候选，没有可渲染的东西"});

    // 产物落工作机本地缓存根，不落网络盘/UNC（D38）。服务端给的 out_dir_ 是相对路径
    // （<session uuid>/<轮次 uuid>），根由本机定 —— 同一份约定，每台工作机各写各的盘。
    const FSys::path l_local_root =
        core_set::get_set().get_cache_root(FSys::path{ai_light::g_ai_light_local_cache_flag});
    l_local_out_dir              = l_local_root / payload_.out_dir_;
    const auto l_ue_payload      = make_ue_payload(payload_, l_local_root);

    SPDLOG_LOGGER_INFO(
        l_logger, "AI 灯光会话 {} 第 {} 轮：{} 个候选，本地输出目录 {}", payload_.session_id_, payload_.iteration_,
        payload_.candidates_.size(), l_local_out_dir.generic_string()
    );

    if (!FSys::exists(l_local_out_dir)) FSys::create_directories(l_local_out_dir);
    // 上一次的残留报告必须先删：否则 commandlet 崩在写报告之前，我们会把上一轮的结果
    // 当成本轮的回传（候选 uuid 对不上，服务端会判「候选不属于本轮」而整轮 400）。
    const FSys::path l_report_path = l_local_out_dir / ai_light::g_ai_light_render_report_file_name;
    if (FSys::exists(l_report_path)) FSys::remove(l_report_path);

    // 写进 -Params 的是绝对路径那版（l_ue_payload）；UE 照它写文件、照它写报告。
    // UE 侧一行都不用改：它本来就只认载荷给的 PngPath/ExrPath。
    auto l_tmp_path = FSys::write_tmp_file("ai_light", nlohmann::json{l_ue_payload}.dump(), ".json");

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
    SPDLOG_LOGGER_INFO(
        l_logger, "AI 灯光第 {} 轮：读到渲染报告，{} 个候选", payload_.iteration_, l_report.candidates_.size()
    );

    // ---------------------------------------------------------------------
    // 上传产物（D40）。这一步必须在回传报告**之前**做完：服务端 apply_render_report
    // 会按报告里说 rendered 的候选去查那张 PNG 到底在不在算出来的落点上（D41），
    // 而那里的文件正是这里上传过去的。
    //
    // PNG 必传：它缺失就等于这个候选不可判断，所以上传失败要如实把状态改成 failed，
    // 而不是让服务端以为有一张能看的图（美术的判断通道只有这张 PNG）。
    // EXR 尽力而为：传失败/没有都只记日志（D42），前端永远不展示 EXR。
    // ---------------------------------------------------------------------
    for (auto& l_cr : l_report.candidates_) {
      if (l_cr.status_ != ai_light::candidate_render_status::rendered) continue;

      const uuid l_id = l_cr.candidate_id_;
      if (l_id.is_nil()) {
        SPDLOG_LOGGER_WARN(l_logger, "AI 灯光第 {} 轮：报告里有一个候选没有 id，无法上传其产物，按失败处理", payload_.iteration_);
        l_cr.status_ = ai_light::candidate_render_status::failed;
        continue;
      }

      const FSys::path l_png = l_local_out_dir / ai_light::make_candidate_file_name(l_id, false);
      if (!FSys::exists(l_png)) {
        // 报告是 UE 写的，产物却没落盘 —— 不猜原因，如实降级。
        SPDLOG_LOGGER_WARN(
            l_logger, "AI 灯光第 {} 轮：候选 {} 报告为渲染成功，但产物 {} 不存在，按失败处理", payload_.iteration_, l_id,
            l_png.generic_string()
        );
        l_cr.status_ = ai_light::candidate_render_status::failed;
        continue;
      }
      try {
        co_await l_kitsu_client->upload_ai_light_candidate_render(l_id, l_png, false);
      } catch (const std::exception& l_ex) {
        SPDLOG_LOGGER_ERROR(
            l_logger, "AI 灯光第 {} 轮：候选 {} 的显示 PNG 上传失败，按失败处理：{}", payload_.iteration_, l_id,
            l_ex.what()
        );
        l_cr.status_ = ai_light::candidate_render_status::failed;
        continue;
      }

      const FSys::path l_exr = l_local_out_dir / ai_light::make_candidate_file_name(l_id, true);
      if (!FSys::exists(l_exr)) {
        SPDLOG_LOGGER_WARN(
            l_logger, "AI 灯光第 {} 轮：候选 {} 没有线性 EXR（{}），不阻断本轮", payload_.iteration_, l_id,
            l_exr.generic_string()
        );
        continue;
      }
      try {
        co_await l_kitsu_client->upload_ai_light_candidate_render(l_id, l_exr, true);
      } catch (const std::exception& l_ex) {
        SPDLOG_LOGGER_WARN(
            l_logger, "AI 灯光第 {} 轮：候选 {} 的线性 EXR 上传失败（不阻断本轮）：{}", payload_.iteration_, l_id,
            l_ex.what()
        );
      }
    }
  } catch (const std::exception& l_ex) {
    SPDLOG_LOGGER_ERROR(l_logger, "AI 灯光渲染任务异常: {}", l_ex.what());
    l_error_msg = l_ex.what();
    // 异常也要回传一份「整轮失败」的报告：服务端靠 error_ 给会话写 failure_reason_，
    // 没有候选就代表一张都没渲染成功（apply_render_report 的两条出口之一）。
    if (l_report.error_.empty()) l_report.error_ = l_error_msg;
    if (!l_local_out_dir.empty())
      SPDLOG_LOGGER_INFO(l_logger, "AI 灯光本地产物保留在 {}", l_local_out_dir.generic_string());
  }

  try {
    co_await l_kitsu_client->post_ai_light_render_report(task_info_.uuid_id_, nlohmann::json{l_report});
  } catch (const std::exception& l_ex) {
    SPDLOG_LOGGER_ERROR(l_logger, "回传渲染报告失败: {}", l_ex.what());
    if (l_error_msg.empty()) l_error_msg = l_ex.what();
  }

  // 本地产物**不删**（D43）：上传成功也不清 —— 出问题时它是唯一的现场，
  // 磁盘增长交给人工/后续定时清理。这里只把路径打进日志，方便事后去捡。
  if (!l_local_out_dir.empty())
    SPDLOG_LOGGER_INFO(l_logger, "AI 灯光第 {} 轮结束，本地产物目录（保留不删）：{}", payload_.iteration_, l_local_out_dir.generic_string());

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
