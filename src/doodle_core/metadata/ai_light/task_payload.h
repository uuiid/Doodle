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
//   D52 —— 引擎内直跑：UE 内置插件可以在**编辑器进程内**完成一轮渲染（进程内 MRQ + 世界回滚），
//          不再依赖外挂 UnrealEditor-Cmd。面板本身**不连网络**（见 D53）。
//   D53 —— 编辑器面板与网络解耦：面板把「本轮该怎么渲」写成 ai_light_launch_plan_t 交给**本机
//          的 Doodle 客户端**，由它起 UE、收产物、上传、回传。理由：UE 的 FHttpManager 只在引擎
//          模块加载期读证书配置（HttpManager.cpp:340-404 / ModuleManager.cpp:1365-1368），
//          运行时挂自签证书钩子的时序不可靠；而 LLM 与统计本来就必须留在服务端。
//   D54 —— 本地回传走 POST /api/doodle/ai/light/render_report_local/{session_id}/{iteration}：
//          这条路线没有 server_task_info 行，所以 apply_render_report 的任务 id 参数可空
//          （空 = 按会话 + 轮次定位）。D41/D42 的产物校验逻辑完全复用，不复制。
//   D57 —— **装配工程必须先同步到工作机本地缓存**（m00148/m00192）：UE 工程不是共享盘上的东西，
//          D:/sy_magic/ue_projects/{code}/EP{ep:04}/{stem} 是**每台工作机各自的缓存**，共享盘
//          （UNC）上的素材必须差分拷贝进这份缓存才能用 —— 这是 auto_light 组装链路一直在做的事
//          （import_and_render_ue.cpp 起 UE 前逐条 copy_diff；工作机侧同一动作见 task_sync.cpp），
//          而且必须这么做：多台机器同时打开同一个工程是非法的。
//          AI 灯光以前把服务端算出的绝对路径直接塞给工作机，工作机上必然没有那份缓存 ——
//          所以载荷里多出 assembly_files_ 与 project_dir_ 两项：
//            * assembly_files_：{from_ = 共享盘绝对路径（只读源），to_ = 相对工程根}；
//            * project_dir_：工程根**相对 core_set::get_set().user_work_root_**（与 task_sync 的
//              "{code}/{归档编号}/{uproj stem}" 口径逐字一致）。
//          入口是工作机自算的（用户决定）：根由本机定，服务端只给相对形状 —— 与 D38/D39 同一条
//          原则，只是这次连"从哪取"也要一并下发（装配源不在工作机上）。
//          这两个键是**必需键**：没有老载荷要兼容（D57 落地时链路还没上生产），
//          from_json 直接 at() 取值，缺键即抛 —— 别为了"宽容"再退回 contains。
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
// 装配文件同步（D57）
// ---------------------------------------------------------------------------

/// 一条装配文件同步项：把共享盘/UNC 上的装配素材拷进工作机本地的 UE 工程缓存。
///
/// 为什么不复用 `import_and_render_ue_ns::file_copy_info`：那个类型的 `to_` 是
/// **服务端缓存根的绝对路径**（`run_ue_assembly_arg::ue_asset_path_` 的原始语义，
/// 由 shot_render_light 在服务端算好、供服务端自己跑组装用）；而这里要下发的是
/// **相对工作机工程根**的路径 —— 根由工作机定（D57/D38）。同名不同义的两个 `to_`
/// 混在一个类型里，迟早会把服务端盘符写进载荷。
struct DOODLE_CORE_API ai_light_assembly_file_t {
  /// 只读源：共享盘/UNC 上的绝对路径（工作机直接读，不回写）。
  FSys::path from_{};
  /// 目标：**相对工程根**（`ai_light_task_payload_t::project_dir_` 那一级）的路径。
  /// 必须是相对路径；服务端在打包清单时已经减掉了工程根前缀。
  FSys::path to_{};

  friend void to_json(nlohmann::json& j, const ai_light_assembly_file_t& p) {
    j["from"] = p.from_;
    j["to"]   = p.to_;
  }
  friend void from_json(const nlohmann::json& j, ai_light_assembly_file_t& p) {
    j.at("from").get_to(p.from_);
    j.at("to").get_to(p.to_);
  }
  bool operator==(const ai_light_assembly_file_t& in_other) const {
    return from_ == in_other.from_ && to_ == in_other.to_;
  }
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
  /// 目标 rig。**含 ops_**：前端已按语义展开候选、`diff_rigs` 已算出与基线的差，
  /// commandlet 直接 apply_ops 即可（§13.4）。
  /// （从前这一步由服务端的 expand_candidates 做，它随候选生成降级一起删除。）
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

/// 本地渲染计划（D53）：/iterate 把它一并返回给**发起方本机**，本机自己起 UE 渲染 ——
/// 不再经过 server_task_info 队列与工作机 WS。
///
/// 为什么单独一个类型而不是复用 ai_light_task_payload_t：
///   * 队列载荷是「服务端 → 工作机」的单向投递，里面有 baseline_level_ 之类
///     只有工作机才需要的字段，且**落地时必然伴随一列 server_task_info**；
///   * 本地计划是**请求的应答的一部分**，不含任何任务行，也不该让客户端看到任务概念。
/// 形状与 ai_light_task_payload_t 的渲染段逐字一致（out_dir_ + 候选四路径 + rig），
/// 这样两侧可以共用同一份「拼本地根 → 写 -Params」的代码（D51：位置只从载荷取，不重算）。
///
/// 路径语义与 D39 相同：全部**相对**本机缓存根（g_ai_light_local_cache_flag），
/// 发起方拼上自己的根之后再交给 UE。
struct DOODLE_CORE_API ai_light_launch_plan_t {
  uuid session_id_{};
  uuid iteration_id_{};
  std::int32_t iteration_{0};
  /// 本轮目录，相对本机缓存根（D39）。
  FSys::path out_dir_{};
  /// 渲染关卡包路径（D50：空 = 本轮不可渲染，服务端在响应里直接失败）。
  std::string render_map_{};
  /// auto_light 生成好的 level sequence 包路径（**空 = 本轮不可渲染**，D50）。
  std::string level_sequence_{};
  /// auto_light 生成好的 MRQ 配置资产（可空，D49）。
  std::string movie_pipeline_config_{};
  /// 帧区间，只作诊断（D48）。
  std::int32_t frame_begin_{0};
  std::int32_t frame_end_{0};
  /// .uproject 绝对路径（本机起 UE 的第一个参数）。
  /// **队列路线**：服务端只放**工程文件名**（不是路径）—— 工作机按 `project_dir_` + 本机
  /// `user_work_root_` 自算工程根再挂上它（D57）。
  /// **引擎内路线**：发起方就是本机，这个路径是真的。
  std::string ue_project_path_{};
  /// 与 auto_light 同源的相机文件（视频路径不用它，见 camera_file_path_ 的说明）。
  std::string camera_file_path_{};
  /// 装配工程根，**相对 `core_set::get_set().user_work_root_`**（D57）：
  /// `{项目code}/{场景归档编号}/{uproj stem}` —— 与 task_sync 的本地缓存口径逐字一致，
  /// 所以同一条镜头的两条链路（人工组装 / AI 灯光）命中同一份本地缓存。
  FSys::path project_dir_{};
  /// 起 UE 之前要同步进本地缓存的装配素材（D57）：`from_` = 共享盘/UNC 绝对路径，
  /// `to_` = **相对 `project_dir_`** 的路径。空清单 = 本镜头没有要同步的东西。
  std::vector<ai_light_assembly_file_t> assembly_files_{};
  std::vector<ai_light_task_candidate_t> candidates_{};

  friend void to_json(nlohmann::json& j, const ai_light_launch_plan_t& p) {
    j["session_id"]            = p.session_id_;
    j["iteration_id"]          = p.iteration_id_;
    j["iteration"]             = p.iteration_;
    j["out_dir"]               = p.out_dir_;
    j["render_map"]            = p.render_map_;
    j["level_sequence"]        = p.level_sequence_;
    j["movie_pipeline_config"] = p.movie_pipeline_config_;
    j["frame_begin"]           = p.frame_begin_;
    j["frame_end"]             = p.frame_end_;
    j["ue_project_path"]       = p.ue_project_path_;
    j["camera_file_path"]      = p.camera_file_path_;
    j["project_dir"]           = p.project_dir_;
    j["assembly_files"]        = p.assembly_files_;
    j["candidates"]            = p.candidates_;
  }
  friend void from_json(const nlohmann::json& j, ai_light_launch_plan_t& p) {
    if (j.contains("session_id")) j.at("session_id").get_to(p.session_id_);
    if (j.contains("iteration_id")) j.at("iteration_id").get_to(p.iteration_id_);
    if (j.contains("iteration")) j.at("iteration").get_to(p.iteration_);
    if (j.contains("out_dir")) j.at("out_dir").get_to(p.out_dir_);
    if (j.contains("render_map")) j.at("render_map").get_to(p.render_map_);
    if (j.contains("level_sequence")) j.at("level_sequence").get_to(p.level_sequence_);
    if (j.contains("movie_pipeline_config")) j.at("movie_pipeline_config").get_to(p.movie_pipeline_config_);
    if (j.contains("frame_begin")) j.at("frame_begin").get_to(p.frame_begin_);
    if (j.contains("frame_end")) j.at("frame_end").get_to(p.frame_end_);
    if (j.contains("ue_project_path")) j.at("ue_project_path").get_to(p.ue_project_path_);
    if (j.contains("camera_file_path")) j.at("camera_file_path").get_to(p.camera_file_path_);
    j.at("project_dir").get_to(p.project_dir_);
    j.at("assembly_files").get_to(p.assembly_files_);
    if (j.contains("candidates")) j.at("candidates").get_to(p.candidates_);
  }
};

/// ai_light 任务的 command_ 载荷。服务端写 ⇒ 工作机读 ⇒（同一份 JSON 落成临时文件）
/// commandlet 读。**只有这一条载荷通道**：工作机不查库、不连数据库。
struct DOODLE_CORE_API ai_light_task_payload_t {
  uuid session_id_{};
  std::int32_t iteration_{0};
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
  /// **队列路线**：服务端只放**工程文件名**（不是路径）—— 工作机按 `project_dir_` 与本机
  /// `user_work_root_` 自算工程根再挂上它（D57）。引擎内路线由发起方本机填全路径。
  std::string ue_project_path_{};
  /// 与 auto_light 同源的相机文件；空则由 commandlet 用关卡内相机。
  /// 视频路径不用它（相机在序列里），只有将来回退到静帧路径时才用得上。
  std::string camera_file_path_{};
  /// 装配工程根，**相对 `core_set::get_set().user_work_root_`**（D57）：
  /// `{项目code}/{场景归档编号}/{uproj stem}`，与 task_sync 的本地缓存口径一致。
  FSys::path project_dir_{};
  /// 起 UE 之前要同步进本地缓存的装配素材（D57）：`from_` = 共享盘/UNC 绝对路径，
  /// `to_` = **相对 `project_dir_`** 的路径。空清单 = 本镜头没有要同步的东西。
  std::vector<ai_light_assembly_file_t> assembly_files_{};
  std::vector<ai_light_task_candidate_t> candidates_{};

  friend void to_json(nlohmann::json& j, const ai_light_task_payload_t& p) {
    j["session_id"]            = p.session_id_;
    j["iteration"]             = p.iteration_;
    j["out_dir"]               = p.out_dir_;
    j["baseline_level"]        = p.baseline_level_;
    j["render_map"]            = p.render_map_;
    j["level_sequence"]        = p.level_sequence_;
    j["movie_pipeline_config"] = p.movie_pipeline_config_;
    j["frame_begin"]           = p.frame_begin_;
    j["frame_end"]             = p.frame_end_;
    j["ue_project_path"]       = p.ue_project_path_;
    j["camera_file_path"]      = p.camera_file_path_;
    j["project_dir"]           = p.project_dir_;
    j["assembly_files"]        = p.assembly_files_;
    j["candidates"]            = p.candidates_;
  }
  friend void from_json(const nlohmann::json& j, ai_light_task_payload_t& p) {
    if (j.contains("session_id")) j.at("session_id").get_to(p.session_id_);
    if (j.contains("iteration")) j.at("iteration").get_to(p.iteration_);
    if (j.contains("out_dir")) j.at("out_dir").get_to(p.out_dir_);
    if (j.contains("baseline_level")) j.at("baseline_level").get_to(p.baseline_level_);
    if (j.contains("render_map")) j.at("render_map").get_to(p.render_map_);
    if (j.contains("level_sequence")) j.at("level_sequence").get_to(p.level_sequence_);
    if (j.contains("movie_pipeline_config")) j.at("movie_pipeline_config").get_to(p.movie_pipeline_config_);
    if (j.contains("frame_begin")) j.at("frame_begin").get_to(p.frame_begin_);
    if (j.contains("frame_end")) j.at("frame_end").get_to(p.frame_end_);
    if (j.contains("ue_project_path")) j.at("ue_project_path").get_to(p.ue_project_path_);
    if (j.contains("camera_file_path")) j.at("camera_file_path").get_to(p.camera_file_path_);
    j.at("project_dir").get_to(p.project_dir_);
    j.at("assembly_files").get_to(p.assembly_files_);
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
    j.at("candidate_id").get_to(p.candidate_id_);
    j.at("status").get_to(p.status_);
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
    j.at("candidates").get_to(p.candidates_);
    if (j.contains("frame_rate")) j.at("frame_rate").get_to(p.frame_rate_);
    if (j.contains("error")) j.at("error").get_to(p.error_);
  }
};

}  // namespace doodle::ai_light
