//
// AI 灯光：派发载荷与回传载荷（server_task_info.command_ 的 wire 契约）
//
// 为什么单独一个头：这两组结构体的唯一用途就是**跨进程传输** ——
//   服务端（http_method/kitsu/ai_light_orchestrator.cpp 写、ai_light.cpp 收）、
//   工作机（exe_warp/ai_light_task.cpp 读与写）、
//   UE commandlet（-Params=<file> 读、<out_dir>/render_report.json 写）。
// 放在 doodle_core 是为了让两侧用**同一份** to_json / from_json：任何一侧自己拼
// 字符串，就会出现「字段改名后另一侧静默取到默认值」的漂移，而那种故障只在真机
// 派发时才暴露。所以键名宁可冻结，并用单测锁死
// （test/core/ai_light_dispatch.cpp 的 task_payload_json_keys_are_frozen）。
//
// 路径策略：
//   D29 —— 所有输出路径由**服务端**算好写进载荷，工作机与 commandlet 只按给的
//          路径写文件，不自己拼目录（消除三方命名漂移）。
//   D27 —— 服务端落库时**不信任**回传里的任何路径，一律用
//          kitsu_ctx_t::get_ai_light_candidate_file 重算；回传只带数值与状态。
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/ai_light/candidate.h>
#include <doodle_core/metadata/ai_light/common.h>
#include <doodle_core/metadata/ai_light/rig.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace doodle::ai_light {

/// commandlet 把整轮渲染结果写成这个文件名，放在载荷 out_dir_ 里。
/// 三方共用（服务端不读它、工作机读、UE 写），所以必须只有一个定义。
constexpr std::string_view g_ai_light_render_report_file_name{"render_report.json"};

// ---------------------------------------------------------------------------
// 派发：服务端 → 工作机 → commandlet
// ---------------------------------------------------------------------------

/// 派发载荷里的一个候选。
struct DOODLE_CORE_API ai_light_task_candidate_t {
  /// 第几个候选（0 = 基线）。同时是输出文件名的序号（见 get_ai_light_candidate_file）。
  std::int32_t candidate_index_{0};
  uuid candidate_id_{};
  /// 输出路径由服务端算好（D29）：display 空间 PNG + 线性 EXR 双输出（§7.4）。
  FSys::path png_path_{};
  FSys::path exr_path_{};
  /// 目标 rig。**含 ops_**：expand_candidates 已按轴展开、diff_rigs 已算出与基线的差，
  /// commandlet 直接 apply_ops 即可（§13.4），不需要它自己再算轴。
  lighting_rig_t rig_{};

  friend void to_json(nlohmann::json& j, const ai_light_task_candidate_t& p) {
    j["candidate_index"] = p.candidate_index_;
    j["candidate_id"]    = p.candidate_id_;
    j["png_path"]        = p.png_path_;
    j["exr_path"]        = p.exr_path_;
    j["rig"]             = p.rig_;
  }
  friend void from_json(const nlohmann::json& j, ai_light_task_candidate_t& p) {
    if (j.contains("candidate_index")) j.at("candidate_index").get_to(p.candidate_index_);
    if (j.contains("candidate_id")) j.at("candidate_id").get_to(p.candidate_id_);
    if (j.contains("png_path")) j.at("png_path").get_to(p.png_path_);
    if (j.contains("exr_path")) j.at("exr_path").get_to(p.exr_path_);
    if (j.contains("rig")) j.at("rig").get_to(p.rig_);
  }
};

/// ai_light 任务的 command_ 载荷。服务端写 ⇒ 工作机读 ⇒（同一份 JSON 落成临时文件）
/// commandlet 读。**只有这一条载荷通道**：工作机不查库、不连数据库。
struct DOODLE_CORE_API ai_light_task_payload_t {
  uuid session_id_{};
  std::int32_t iteration_{0};
  uuid spec_id_{};
  /// 本轮目录。commandlet 在这里写 g_ai_light_render_report_file_name，工作机在这里读。
  FSys::path out_dir_{};
  /// 基线关卡（复制源；**源资产永不修改**，§13.4）。
  std::string baseline_level_{};
  /// .uproject 绝对路径（UE 命令行的第一个参数）。
  std::string ue_project_path_{};
  /// 与 auto_light 同源的相机文件；空则由 commandlet 用关卡内相机。
  std::string camera_file_path_{};
  std::vector<ai_light_task_candidate_t> candidates_{};

  friend void to_json(nlohmann::json& j, const ai_light_task_payload_t& p) {
    j["session_id"]        = p.session_id_;
    j["iteration"]         = p.iteration_;
    j["spec_id"]           = p.spec_id_;
    j["out_dir"]           = p.out_dir_;
    j["baseline_level"]    = p.baseline_level_;
    j["ue_project_path"]   = p.ue_project_path_;
    j["camera_file_path"]  = p.camera_file_path_;
    j["candidates"]        = p.candidates_;
  }
  friend void from_json(const nlohmann::json& j, ai_light_task_payload_t& p) {
    if (j.contains("session_id")) j.at("session_id").get_to(p.session_id_);
    if (j.contains("iteration")) j.at("iteration").get_to(p.iteration_);
    if (j.contains("spec_id")) j.at("spec_id").get_to(p.spec_id_);
    if (j.contains("out_dir")) j.at("out_dir").get_to(p.out_dir_);
    if (j.contains("baseline_level")) j.at("baseline_level").get_to(p.baseline_level_);
    if (j.contains("ue_project_path")) j.at("ue_project_path").get_to(p.ue_project_path_);
    if (j.contains("camera_file_path")) j.at("camera_file_path").get_to(p.camera_file_path_);
    if (j.contains("candidates")) j.at("candidates").get_to(p.candidates_);
  }
};

// ---------------------------------------------------------------------------
// 回传：工作机 → 服务端
// ---------------------------------------------------------------------------

/// 单个候选的渲染结果。**不带路径**（D27：服务端重算），只带状态与数值。
struct DOODLE_CORE_API candidate_render_report_t {
  std::int32_t candidate_index_{0};
  uuid candidate_id_{};
  candidate_render_status status_{candidate_render_status::rendered};
  std::int64_t render_ms_{0};
  std::int32_t light_count_{0};
  /// 可选诊断数据：commandlet 算了就带上，没算就是默认值（D26：形状错只丢弃、不阻断）。
  candidate_metrics_t metrics_{};

  friend void to_json(nlohmann::json& j, const candidate_render_report_t& p) {
    j["candidate_index"] = p.candidate_index_;
    j["candidate_id"]    = p.candidate_id_;
    j["status"]          = p.status_;
    j["render_ms"]       = p.render_ms_;
    j["light_count"]     = p.light_count_;
    j["metrics"]         = p.metrics_;
  }
  friend void from_json(const nlohmann::json& j, candidate_render_report_t& p) {
    if (j.contains("candidate_index")) j.at("candidate_index").get_to(p.candidate_index_);
    if (j.contains("candidate_id")) j.at("candidate_id").get_to(p.candidate_id_);
    if (j.contains("status")) j.at("status").get_to(p.status_);
    if (j.contains("render_ms")) j.at("render_ms").get_to(p.render_ms_);
    if (j.contains("light_count")) j.at("light_count").get_to(p.light_count_);
    if (j.contains("metrics")) j.at("metrics").get_to(p.metrics_);
  }
};

/// 整轮回传。`error_` 非空 = 整轮失败（UE 启动失败 / 超时 / 载荷非法），
/// 此时 candidates_ 可以为空 —— 服务端照样要写 failure_reason_（D28）。
struct DOODLE_CORE_API round_render_report_t {
  std::vector<candidate_render_report_t> candidates_{};
  std::string error_{};

  friend void to_json(nlohmann::json& j, const round_render_report_t& p) {
    j["candidates"] = p.candidates_;
    j["error"]      = p.error_;
  }
  friend void from_json(const nlohmann::json& j, round_render_report_t& p) {
    if (j.contains("candidates")) j.at("candidates").get_to(p.candidates_);
    if (j.contains("error")) j.at("error").get_to(p.error_);
  }
};

}  // namespace doodle::ai_light
