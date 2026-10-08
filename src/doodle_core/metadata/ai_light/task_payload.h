//
// AI 灯光：派发载荷与回传载荷（server_task_info.command_ 的 wire 契约）
//
// 为什么单独一个头：这两组结构体的唯一用途就是**跨进程传输** ——
//   服务端（http_method/kitsu/ai_light_orchestrator.cpp 写、ai_light.cpp 收）、
//   工作机（exe_warp/ai_light_render.cpp 读与写）、
//   UE commandlet（-Params=<file> 读、<out_dir>/render_report.json 写）。
// 放在 doodle_core 是为了让两侧用**同一份** to_json / from_json：任何一侧自己拼
// 字符串，就会出现「字段改名后另一侧静默取到默认值」的漂移，而那种故障只在真机
// 派发时才暴露。所以键名宁可冻结，并用单测锁死
// （test/core/ai_light_dispatch.cpp 的 task_payload_json_keys_are_frozen）。
//
// 产物与路径策略：候选产物落在**工作机本地**，再 HTTP 上传（不直写网络盘/UNC）。
//   D38 —— 目录归属：http::kitsu_ctx_t 派生出来的目录（kitsu_ctx_t::root_ 底下这一片）
//          只保证同一台机器同卷，**不是**共享盘；工作机的产物一律写进自己的工作机本地
//          缓存根 core_set::get_set().get_cache_root(g_ai_light_local_cache_flag)。
//          只有 core/entity_path.cpp 派生出来的项目/资产目录才在网络盘/UNC 上。
//   D39 —— 载荷里的路径一律是**相对路径**：out_dir_ = <session_uuid>/<轮次 uuid>，候选的
//          frames_dir_ / png_path_ / exr_path_ / movie_path_ 都相对 out_dir_（命名见
//          make_candidate_frames_dir / make_candidate_file_name / make_candidate_movie_file）。
//          命名由服务端定、根由工作机定；工作机在起 UE 之前把两者拼成绝对路径（写进 -Params
//          的那份 JSON）。
//   D40 —— 产物由工作机 POST 到
//          /api/doodle/ai/light/candidate/{id}/file/{render|render_linear|render_movie}；
//          服务端把收到的文件落到**按命名规则算出来的**那个位置（见 D44）。
//   D45 —— 候选产物是**视频**：渲染交给 MRQ，用 auto_light 已经生成好的关卡（render_map_）、
//          level sequence（level_sequence_，带相机与角色动画）与 movie pipeline 配置
//          （movie_pipeline_config_，可空）；不再由我们自己逐帧驱动序列。
//          每个候选的产物 = 帧序列目录 + mp4 + 首帧海报（海报语义见 png_path_）。
//   D46 —— rig 注入：MRQ 在当前世界渲染，所以一个 UE 进程内串行 6 次
//          「重建 rig → AllocateJob/RenderJob → 等 OnRenderFinished」；序列里的灯光 spawnable
//          在内存里剥掉（否则会和候选 rig 抢光照），序列**永不落盘**。
//   D47 —— 帧序列由 UE 写（UE 只写帧），mp4 与海报由工作机合成/复制；帧率取回传的 frame_rate_
//          （＝序列 display rate），所以它必须真的回传，不能用约定值代替。
//   D48 —— 帧区间＝序列自己的 playback range；frame_begin_ / frame_end_ 只作诊断，必要时才用
//          bUseCustomPlaybackRange + CustomStartFrame/CustomEndFrame 覆盖。
//   D49 —— MRQ 质量设置对齐 auto_light 的 OnSaveReanderConfig（PNG 图像输出、TSR、预热 128 帧、
//          关运动模糊、cinematic 画质）；有 movie_pipeline_config_ 就复制它再用（沿用分辨率等），
//          缺失或加载失败就自建配置（1920×1080）。
//   D50 —— level_sequence_ 为空 = 本轮不可渲染：服务端派发时直接失败（走回滚写 failure_reason_），
//          不回退到静帧路径。
//   D27 —— 服务端不信任回传里的任何路径（回传里只有数值与状态）：产物位置一律按
//          kitsu_ctx_t 的命名规则现算。
//   D41 —— 回传说 rendered 的候选，服务端按命名规则算出海报与 mp4 两个路径，再查文件是否真的在：
//          任一不在就降级成 failed（否则美术会对着取不到的产物点「选第 3 个」）。
//   D42 —— 线性 EXR 是**可选**产物（本轮不产）：将来若产出，工作机尽量传，传不上去 / 文件不在
//          都只 WARN、不阻断本轮。
//   D43 —— 工作机本地产物**保留不删**：上传成功也不清，出问题时它是唯一的现场；
//          只在日志里报路径，磁盘增长留给人工/后续定时清理。
//   D44 —— 候选**不存渲染路径**：它只带 session_id_ / iteration_id_ / uuid_id_ 三个 uuid，
//          路径由 doodle_lib 的 candidate_frames_dir / candidate_render_path / candidate_movie_path
//          现算（都是 kitsu_ctx_t 对应 getter 的薄包装）；候选 JSON 里也没有路径键 ——
//          前端取图与取视频只按 uuid 走 /candidate/{uuid}/image 与 /candidate/{uuid}/movie。
//          代价：库里没有「文件实际在哪」的记录（root_ 变了而文件没搬时无从察觉）；
//          若将来出现**非派生**产物（人工补图、对象存储），需要重新引入一个显式的路径字段。
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

/// 工作机本地产物根：`core_set::get_set().get_cache_root(g_ai_light_local_cache_flag)`（D38）。
/// 与 kitsu_ctx_t::root_ 那一套（"ai_light" 也是它的一级目录）**故意同名**：同名不代表同盘，
/// 一个是工作机缓存根，一个是服务端数据根，唯一共同点是都以 "ai_light" 分片。
constexpr std::string_view g_ai_light_local_cache_flag{"ai_light"};

/// 本轮相对目录（D39）：`<session_uuid>/<轮次 uuid>`（相对工作机缓存根）。
/// 末一级与服务端 kitsu_ctx_t::get_ai_light_iteration_dir 同名（都是轮次 uuid），
/// 方便人对照两边目录。
inline std::string make_iteration_relative_dir(const uuid& in_session_id, const uuid& in_iteration_id) {
  return fmt::format("{}/{}", in_session_id, in_iteration_id);
}

/// 候选产物相对文件名（D39）：`<candidate_uuid>.png` / `.exr`。
/// **这个文件名是两侧共用的契约**：服务端算落点用的
/// `kitsu_ctx_t::get_ai_light_candidate_file` 生成同一个字符串，改动必须同步改两处。
inline std::string make_candidate_file_name(const uuid& in_candidate_id, bool in_linear) {
  return fmt::format("{}{}", in_candidate_id, in_linear ? ".exr" : ".png");
}

/// 候选帧序列的相对目录（D47）：`<candidate_uuid>/`（相对 out_dir_）。
/// MRQ 把这一轮的帧全写在这里，工作机再把它合成成 mp4。
/// 与服务端 `kitsu_ctx_t::get_ai_light_candidate_frames_dir` 是同一个契约。
inline std::string make_candidate_frames_dir(const uuid& in_candidate_id) { return fmt::format("{}/", in_candidate_id); }

/// 候选视频的相对文件名（D47）：`<candidate_uuid>.mp4`（相对 out_dir_）。
/// 与服务端 `kitsu_ctx_t::get_ai_light_candidate_movie_file` 是同一个契约。
inline std::string make_candidate_movie_file(const uuid& in_candidate_id) {
  return fmt::format("{}.mp4", in_candidate_id);
}

/// 候选产物的种类（D50）：上传路由的 URL 段与服务端落点、工作机上传调用共用同一个枚举，
/// 所以它定义在契约头里而不是某一侧的路由头里。
/// 枚举名会进 URL（路由正则由 magic_enum 枚举名拼成），**改名等于改外部契约**。
enum class ai_light_candidate_file_role {
  render,         ///< 海报 PNG（display 空间，前端网格与定稿都用它）
  render_linear,  ///< 线性 EXR（可选产物，本轮不产）
  render_movie,   ///< mp4（美术判断光照的主要依据）
};

// ---------------------------------------------------------------------------
// 派发：服务端 → 工作机 → commandlet
// ---------------------------------------------------------------------------

/// 派发载荷里的一个候选。
struct DOODLE_CORE_API ai_light_task_candidate_t {
  /// 第几个候选（0 = 基线）。**只用于排序与显示**：产物文件名用的是 candidate_id_，
  /// 不是这个序号（文件名契约见 make_candidate_file_name）。
  std::int32_t candidate_index_{0};
  uuid candidate_id_{};
  /// **相对路径**（相对 out_dir_，D39）：MRQ 把这一轮渲出来的帧写进 frames_dir_（D47），
  /// 工作机把它合成成 movie_path_，再把首帧复制成 png_path_ 当海报。
  /// exr_path_ 是线性 EXR 的预留位（可选产物，本轮不产）。
  /// 四个路径都由服务端的 make_candidate_* 生成，工作机拼上本地根之后才交给 UE。
  FSys::path frames_dir_{};
  FSys::path png_path_{};
  FSys::path exr_path_{};
  FSys::path movie_path_{};
  /// 目标 rig。**含 ops_**：expand_candidates 已按轴展开、diff_rigs 已算出与基线的差，
  /// commandlet 直接 apply_ops 即可（§13.4），不需要它自己再算轴。
  lighting_rig_t rig_{};

  friend void to_json(nlohmann::json& j, const ai_light_task_candidate_t& p) {
    j["candidate_index"] = p.candidate_index_;
    j["candidate_id"]    = p.candidate_id_;
    j["frames_dir"]      = p.frames_dir_;
    j["png_path"]        = p.png_path_;
    j["exr_path"]        = p.exr_path_;
    j["movie_path"]      = p.movie_path_;
    j["rig"]             = p.rig_;
  }
  friend void from_json(const nlohmann::json& j, ai_light_task_candidate_t& p) {
    if (j.contains("candidate_index")) j.at("candidate_index").get_to(p.candidate_index_);
    if (j.contains("candidate_id")) j.at("candidate_id").get_to(p.candidate_id_);
    if (j.contains("frames_dir")) j.at("frames_dir").get_to(p.frames_dir_);
    if (j.contains("png_path")) j.at("png_path").get_to(p.png_path_);
    if (j.contains("exr_path")) j.at("exr_path").get_to(p.exr_path_);
    if (j.contains("movie_path")) j.at("movie_path").get_to(p.movie_path_);
    if (j.contains("rig")) j.at("rig").get_to(p.rig_);
  }
};

/// ai_light 任务的 command_ 载荷。服务端写 ⇒ 工作机读 ⇒（同一份 JSON 落成临时文件）
/// commandlet 读。**只有这一条载荷通道**：工作机不查库、不连数据库。
struct DOODLE_CORE_API ai_light_task_payload_t {
  uuid session_id_{};
  std::int32_t iteration_{0};
  uuid spec_id_{};
  /// 本轮目录，**相对工作机本地缓存根**（D39）。commandlet 在这里写
  /// g_ai_light_render_report_file_name，工作机在这里读 —— 两边拿到的都是绝对路径，
  /// 因为工作机起 UE 之前已经把它拼成了绝对路径（载荷本身只带相对路径）。
  FSys::path out_dir_{};
  /// 基线关卡（复制源；**源资产永不修改**，§13.4）。
  std::string baseline_level_{};
  /// 渲染关卡（**包路径**，如 `/Game/Shot/ep0001/xxx_JS/sc001_JS`）：auto_light 装配时算出来的
  /// 交付场景关卡，相机与角色都在这里。空的退化情形由 baseline_level_ 兜底（静态路径用）。
  std::string render_map_{};
  /// auto_light 生成好的 level sequence（**包路径**）：相机轨道 + camera cut + 角色动画都在里面，
  /// MRQ 渲染它（D45）。**空 = 本轮不可渲染**（D50）。
  std::string level_sequence_{};
  /// auto_light 生成好的 MRQ 配置资产（**包路径**，可空）：有就复制它再用（沿用分辨率与画质），
  /// 没有就自建配置（D49）。
  std::string movie_pipeline_config_{};
  /// 镜头帧区间，**只作诊断**（D48）：真正渲染的范围由序列的 playback range 决定。
  std::int32_t frame_begin_{0};
  std::int32_t frame_end_{0};
  /// .uproject 绝对路径（UE 命令行的第一个参数）。
  std::string ue_project_path_{};
  /// 与 auto_light 同源的相机文件；空则由 commandlet 用关卡内相机。
  /// 视频路径不用它（相机在序列里），只有将来回退到静帧路径时才用得上。
  std::string camera_file_path_{};
  std::vector<ai_light_task_candidate_t> candidates_{};

  friend void to_json(nlohmann::json& j, const ai_light_task_payload_t& p) {
    j["session_id"]            = p.session_id_;
    j["iteration"]             = p.iteration_;
    j["spec_id"]               = p.spec_id_;
    j["out_dir"]               = p.out_dir_;
    j["baseline_level"]        = p.baseline_level_;
    j["render_map"]            = p.render_map_;
    j["level_sequence"]        = p.level_sequence_;
    j["movie_pipeline_config"] = p.movie_pipeline_config_;
    j["frame_begin"]           = p.frame_begin_;
    j["frame_end"]             = p.frame_end_;
    j["ue_project_path"]       = p.ue_project_path_;
    j["camera_file_path"]      = p.camera_file_path_;
    j["candidates"]            = p.candidates_;
  }
  friend void from_json(const nlohmann::json& j, ai_light_task_payload_t& p) {
    if (j.contains("session_id")) j.at("session_id").get_to(p.session_id_);
    if (j.contains("iteration")) j.at("iteration").get_to(p.iteration_);
    if (j.contains("spec_id")) j.at("spec_id").get_to(p.spec_id_);
    if (j.contains("out_dir")) j.at("out_dir").get_to(p.out_dir_);
    if (j.contains("baseline_level")) j.at("baseline_level").get_to(p.baseline_level_);
    if (j.contains("render_map")) j.at("render_map").get_to(p.render_map_);
    if (j.contains("level_sequence")) j.at("level_sequence").get_to(p.level_sequence_);
    if (j.contains("movie_pipeline_config")) j.at("movie_pipeline_config").get_to(p.movie_pipeline_config_);
    if (j.contains("frame_begin")) j.at("frame_begin").get_to(p.frame_begin_);
    if (j.contains("frame_end")) j.at("frame_end").get_to(p.frame_end_);
    if (j.contains("ue_project_path")) j.at("ue_project_path").get_to(p.ue_project_path_);
    if (j.contains("camera_file_path")) j.at("camera_file_path").get_to(p.camera_file_path_);
    if (j.contains("candidates")) j.at("candidates").get_to(p.candidates_);
  }
};

// ---------------------------------------------------------------------------
// 回传：工作机 → 服务端
// ---------------------------------------------------------------------------

/// 单个候选的渲染结果。**不带路径**（D27：服务端按命名规则现算/D41：再查文件在不在），
/// 只带状态与数值。
struct DOODLE_CORE_API candidate_render_report_t {
  std::int32_t candidate_index_{0};
  uuid candidate_id_{};
  candidate_render_status status_{candidate_render_status::rendered};
  std::int64_t render_ms_{0};
  std::int32_t light_count_{0};
  /// MRQ 这一轮实际写出来的帧数（D47）：工作机据此确认帧目录非空，也用来记录成片长度。
  std::int32_t frame_count_{0};
  /// 可选诊断数据：commandlet 算了就带上，没算就是默认值（D26：形状错只丢弃、不阻断）。
  candidate_metrics_t metrics_{};

  friend void to_json(nlohmann::json& j, const candidate_render_report_t& p) {
    j["candidate_index"] = p.candidate_index_;
    j["candidate_id"]    = p.candidate_id_;
    j["status"]          = p.status_;
    j["render_ms"]       = p.render_ms_;
    j["light_count"]     = p.light_count_;
    j["frame_count"]     = p.frame_count_;
    j["metrics"]         = p.metrics_;
  }
  friend void from_json(const nlohmann::json& j, candidate_render_report_t& p) {
    if (j.contains("candidate_index")) j.at("candidate_index").get_to(p.candidate_index_);
    if (j.contains("candidate_id")) j.at("candidate_id").get_to(p.candidate_id_);
    if (j.contains("status")) j.at("status").get_to(p.status_);
    if (j.contains("render_ms")) j.at("render_ms").get_to(p.render_ms_);
    if (j.contains("light_count")) j.at("light_count").get_to(p.light_count_);
    if (j.contains("frame_count")) j.at("frame_count").get_to(p.frame_count_);
    if (j.contains("metrics")) j.at("metrics").get_to(p.metrics_);
  }
};

/// 整轮回传。`error_` 非空 = 整轮失败（UE 启动失败 / 超时 / 载荷非法），
/// 此时 candidates_ 可以为空 —— 服务端照样要写 failure_reason_（D28）。
struct DOODLE_CORE_API round_render_report_t {
  std::vector<candidate_render_report_t> candidates_{};
  /// 帧率（D47）：＝序列的 display rate，工作机用它合成 mp4。
  /// 由 UE 回报而不是服务端约定，因为真正决定帧数的是序列自己的 display rate。
  std::int32_t frame_rate_{0};
  std::string error_{};

  friend void to_json(nlohmann::json& j, const round_render_report_t& p) {
    j["candidates"] = p.candidates_;
    j["frame_rate"] = p.frame_rate_;
    j["error"]      = p.error_;
  }
  friend void from_json(const nlohmann::json& j, round_render_report_t& p) {
    if (j.contains("candidates")) j.at("candidates").get_to(p.candidates_);
    if (j.contains("frame_rate")) j.at("frame_rate").get_to(p.frame_rate_);
    if (j.contains("error")) j.at("error").get_to(p.error_);
  }
};

}  // namespace doodle::ai_light
