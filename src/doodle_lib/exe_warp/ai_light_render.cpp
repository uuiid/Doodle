//
// AI 灯光分布式任务：一轮候选的渲染（工作机侧）
//
// 工作机只做三件事：把载荷里的相对路径拼到本地缓存根、跑一次 UE 让 MRQ 逐候选出帧、
// 把帧合成视频（外加首帧当海报）逐张上传，最后回传报告并落任务终态。
// UE 渲染本身（相机/定序器/逐候选换灯）在 UE 侧，见 docs/ue_ai_lighting_plan.md §7/§15。
//

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

/// 把载荷里那些**相对路径**拼到工作机本地根上，返回一份给 UE 用的副本（D38/D39）。
///
/// 为什么不就地改 payload_：payload_ 是服务端那份「相对命名」契约的本地镜像（wire 解码后的原样），
/// 要绝对路径时**另做一份副本** —— 下面收产物那段就是拿这份副本按候选 id 反查本地产物位置的。
/// 就地改并不会更省事，却会让「工作机本地盘长什么样」粘在这份结构上 —— 将来谁把它写进日志
/// 或回传，就把工作机的本地路径漏给服务端了，而服务端既用不到也不该知道（D38）。
ai_light::ai_light_task_payload_t make_ue_payload(
    const ai_light::ai_light_task_payload_t& in_payload, const FSys::path& in_local_root
) {
  ai_light::ai_light_task_payload_t l_out = in_payload;
  const FSys::path l_out_dir              = in_local_root / in_payload.out_dir_;
  l_out.out_dir_                          = l_out_dir;
  for (auto& l_c : l_out.candidates_) {
    // 这几个相对路径相对的是 out_dir_，不是本地根，所以和 l_out_dir 拼。
    l_c.frames_dir_ = l_out_dir / l_c.frames_dir_;
    l_c.png_path_   = l_out_dir / l_c.png_path_;
    l_c.exr_path_   = l_out_dir / l_c.exr_path_;
    l_c.movie_path_ = l_out_dir / l_c.movie_path_;
  }
  return l_out;
}

/// 收集 MRQ 写进帧目录的 PNG 并排序。
///
/// 文件名形如 frame_0000.png（补零），所以按路径排序就是帧序 ——
/// 这里不用 movie::image_attr::extract_num 那套「取前两张文件名里的数字差」的推断：
/// 它要求输入已经是正确顺序，而我们手里唯一的顺序信息就是文件名本身。
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

/// 把共享盘上的装配素材差分拷进**本机的 UE 工程缓存**，返回本机上真正可用的 .uproject 路径（D57）。
///
/// 为什么必须做这一步：UE 工程不是共享盘上的东西，而是**每台机器各一份的本地缓存**。共享盘（UNC）
/// 上的素材是只读源，必须先差分拷贝进本地缓存，才能被引擎打开。这也是唯一可行的做法 ——
/// 多台机器同时打开同一个工程是非法的。
///
/// 口径与人工组装链路（auto_task.cpp 的 tasks/sync 与 import_and_render_ue_ns::run()）逐字一致：
///   根 = core_set::get_set().user_work_root_ / project_dir_（{code}/{归档编号}/{uproj stem}）
///   逐条 = FSys::copy_diff(from_, 根 / to_)
/// 所以同一条镜头无论是人工组装还是 AI 灯光，命中同一份本地缓存，不会各拷一份出来。
///
/// **载荷缺这两项就是错误的载荷**（D57 落地时链路还没上生产，不留兼容口子）：宁可在这里明确报错，
/// 也不要退回服务端算的那条路径 —— 工作机上没有那份缓存，拿去起 UE 只会得到「工程打不开」。
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
  // project_dir_ 是**相对** user_work_root_ 的（D57）。
  const FSys::path l_prj_dir = l_work_root / in_payload.project_dir_;

  if (!FSys::exists(l_prj_dir)) FSys::create_directories(l_prj_dir);

  SPDLOG_LOGGER_INFO(
      in_logger, "AI 灯光第 {} 轮：同步 {} 个装配文件到本地工程 {}", in_payload.iteration_,
      in_payload.assembly_files_.size(), l_prj_dir.generic_string()
  );
  for (auto&& l_file : in_payload.assembly_files_) {
    DOODLE_CHICK(!l_file.from_.empty(), "装配同步清单里有一条空的源路径，无法同步");
    // copy_diff 按 mtime/大小差分：本地已有且不旧的文件会被跳过，所以重复轮次几乎不花时间。
    FSys::copy_diff(l_file.from_, l_prj_dir / l_file.to_, in_logger);
  }

  // 起 UE 用的是**本地**那份 .uproject：载荷里的 ue_project_path_ 只是工程文件名（D57），
  // 挂到本机工程根下 —— 绝不带服务端盘符（那台机器上的路径在这里没有意义）。
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
  // 本轮的本地输出目录。声明在 try 外面是为了在异常路径的日志里也能报出来 ——
  // 本地产物不删（D43），那就得让日志说清去哪儿捡。
  FSys::path l_local_out_dir{};

  try {
    payload_ = task_info_.command_.get<ai_light::ai_light_task_payload_t>();

    // 硬前置。缺任何一个 UE 都渲不出东西 —— 直接当整轮失败回传，
    // 别让服务端那条会话在 generating 里干等一个永远不会来的报告。
    if (payload_.ue_project_path_.empty()) throw_exception(doodle_error{"载荷里没有 .uproject 路径，无法启动 UE"});
    if (payload_.render_map_.empty()) throw_exception(doodle_error{"载荷里没有渲染关卡，无法渲染"});
    // 没有序列就没有镜头运动，也就没有视频可言（D50）。服务端派发前已经挡了一道，
    // 这里是第二道：真拿到一个空序列的载荷，宁可整轮失败也别渲出一堆静帧。
    if (payload_.level_sequence_.empty()) throw_exception(doodle_error{"载荷里没有 level sequence，无法渲染视频"});
    if (payload_.candidates_.empty()) throw_exception(doodle_error{"载荷里没有候选，没有可渲染的东西"});
    if (payload_.movie_pipeline_config_.empty())
      SPDLOG_LOGGER_WARN(l_logger, "AI 灯光第 {} 轮：载荷里没有电影管线配置，UE 会按内置参数另建一份", payload_.iteration_);

    // 产物落工作机本地缓存根，不落网络盘/UNC（D38）。服务端给的 out_dir_ 是相对路径
    // （<session uuid>/<轮次 uuid>），根由本机定 —— 同一份约定，每台工作机各写各的盘。
    const FSys::path l_local_root =
        core_set::get_set().get_cache_root(FSys::path{ai_light::g_ai_light_local_cache_flag});
    // 拼好的这一份是后面**唯一的路径权威**：起 UE 用它（写进 -Params），收产物也从它按候选 id 取。
    const auto l_ue_payload = make_ue_payload(payload_, l_local_root);
    l_local_out_dir         = l_ue_payload.out_dir_;

    SPDLOG_LOGGER_INFO(
        l_logger, "AI 灯光会话 {} 第 {} 轮：{} 个候选，本地输出目录 {}", payload_.session_id_, payload_.iteration_,
        payload_.candidates_.size(), l_local_out_dir.generic_string()
    );

    if (!FSys::exists(l_local_out_dir)) FSys::create_directories(l_local_out_dir);
    // 上一次的残留报告必须先删：否则渲染进程崩在写报告之前，我们会把上一轮的结果
    // 当成本轮的回传（候选 uuid 对不上，服务端会判「候选不属于本轮」而整轮 400）。
    const FSys::path l_report_path = l_local_out_dir / ai_light::g_ai_light_render_report_file_name;
    if (FSys::exists(l_report_path)) FSys::remove(l_report_path);

    // D57：先把装配文件同步进本机工程缓存，再起 UE。这一步在**渲染关卡**算好之前做，
    // 因为它决定的是「本机到底有没有这个工程」——工程都没有，关卡包路径无从谈起。
    const FSys::path l_uproject = sync_assembly_files(payload_, l_logger);
    // 与人工组装链路同样收尾：补 .uproject 的插件段与 Config/DefaultEngine.ini。
    // 共享盘那份工程里的这两处可能是别的机器上的形状，装配完之后必须按本机重写。
    import_and_render_ue_ns::fix_project(l_uproject);
    import_and_render_ue_ns::fix_config(l_uproject);

    // 启动形状与 auto_light 的渲染步一致：**不带 `-run=`**，直接把渲染关卡当第一个位置参数交给编辑器，
    // 由编辑器模块自己读参数、自己起 PIE 渲染（D46）。
    //
    // 为什么不能用 commandlet：MRQ 靠 `FCoreDelegates::OnBeginFrame` 推进帧，而这个委托全引擎只有
    // `FEngineLoop::Tick` 广播一处（`LaunchEngineLoop.cpp:5663`）；commandlet 的
    // `CommandletHelpers::TickEngine` 只 tick 引擎与 Slate，不广播它 —— 管线会被建起来，然后一帧都不渲。
    // 何况 MRQ 的 `MoviePipeline::FindCurrentWorld()` 只认 Game/PIE 世界，commandlet 里也没有。
    // 拷贝初始化，**不要**写 nlohmann::json{l_ue_payload}：花括号走 initializer_list 构造，
    // 写出的是一个单元素数组，UE 侧按对象解析会直接失败。
    const nlohmann::json l_payload_json = l_ue_payload;
    auto l_tmp_path = FSys::write_tmp_file("ai_light", l_payload_json.dump(), ".json");
    const auto l_ue_arg = std::vector<std::string>{
        l_uproject.generic_string(), payload_.render_map_, "-log", "-AllowStdOutLogVerbosity", "-ForceLogFlush",
        "-Unattended", fmt::format("-DoodleAiLightParams={}", l_tmp_path.generic_string())};

    // create_lock = true：UE 是整台机器的独占资源，async_run_ue 内部会拿全局队列锁。
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

    // 帧率必须按序列的 display rate 合成，否则同一段动画会被拉长/压短。
    const std::int32_t l_fps = l_report.frame_rate_ > 0 ? l_report.frame_rate_ : 25;

    // ---------------------------------------------------------------------
    // 逐候选合成视频 + 上传（D40/D47）。这一步必须在回传报告**之前**做完：
    // 服务端 apply_render_report 会按报告里说 rendered 的候选去查海报和视频到底在不在
    // 算出来的落点上（D41），而那里的文件正是这里上传过去的。
    //
    // 海报与视频都必传：缺任何一个这个候选就不可判断（美术靠视频看灯怎么动、
    // 靠海报在网格里认它），所以上传失败要如实把状态改成 failed，
    // 而不是让服务端以为有一份能看的东西。
    // ---------------------------------------------------------------------
    for (auto& l_cr : l_report.candidates_) {
      if (l_cr.status_ != ai_light::candidate_render_status::rendered) continue;

      const uuid l_id = l_cr.candidate_id_;
      if (l_id.is_nil()) {
        SPDLOG_LOGGER_WARN(l_logger, "AI 灯光第 {} 轮：报告里有一个候选没有 id，无法上传其产物，按失败处理", payload_.iteration_);
        l_cr.status_ = ai_light::candidate_render_status::failed;
        continue;
      }

      // 产物位置取自**载荷**（D39）：UE 就是按载荷里那几条绝对路径写盘的
      // （DoodleLightingRoundRunner.cpp:169 把 MRQ 的 OutputDirectory.Path 设成 InCandidate.FramesDir），
      // 而回传报告里只有 candidate_id_，没有路径（D27）。
      //
      // 所以按 **uuid** 反查载荷：下标是弱键，编号一旦有偏差就会把 A 的
      // 成片挂到 B 的 uuid 上，美术对着别人的灯评判 —— 这条链路上最糟的失败形态。
      // 也刻意不按命名规则再算一遍：那等于工作机再实现一次服务端的命名，版本偏斜（键名没变、
      // 命名值变了）时算出来的目录会对不上 UE 实际写的位置，白白把一整轮判失败。
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
        // 报告是 UE 写的，帧却没落盘（或只落了一张）—— 不猜原因，如实降级。
        SPDLOG_LOGGER_WARN(
            l_logger, "AI 灯光第 {} 轮：候选 {} 报告为渲染成功，但帧目录 {} 里只有 {} 张帧，合成不出视频，按失败处理",
            payload_.iteration_, l_id, l_frames_dir.generic_string(), l_frames.size()
        );
        l_cr.status_ = ai_light::candidate_render_status::failed;
        continue;
      }

      try {
        // 首帧当海报：它就是前端网格里那张图，和视频同一时刻，不会对不上。
        // 先用它把尺寸也定下来（create_move 不传尺寸时读第一帧）。
        std::error_code l_ec{};
        FSys::copy_file(l_frames.front(), l_png, FSys::copy_options::overwrite_existing, l_ec);
        if (l_ec) throw_exception(doodle_error{"首帧海报写出失败 {}：{}", l_png.generic_string(), l_ec.message()});

        // 必须全限定：本文件在 namespace doodle 里，`detail::` 会先命中 doodle::http::detail（work.h）。
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
        co_await l_kitsu_client->upload_ai_light_candidate_file(
            l_id, l_png, ai_light::ai_light_candidate_file_role::render
        );
        co_await l_kitsu_client->upload_ai_light_candidate_file(
            l_id, l_movie, ai_light::ai_light_candidate_file_role::render_movie
        );
      } catch (const std::exception& l_ex) {
        SPDLOG_LOGGER_ERROR(
            l_logger, "AI 灯光第 {} 轮：候选 {} 的产物上传失败，按失败处理：{}", payload_.iteration_, l_id, l_ex.what()
        );
        l_cr.status_ = ai_light::candidate_render_status::failed;
        continue;
      }

      // 本轮不产线性 EXR（D42：角色留在契约里，等线性交付那条线要用时再接）。
      SPDLOG_LOGGER_INFO(
          l_logger, "AI 灯光第 {} 轮：候选 {} 完成，{} 帧 @ {} fps，海报 {}，视频 {}", payload_.iteration_, l_id,
          l_cr.frame_count_, l_fps, l_png.generic_string(), l_movie.generic_string()
      );
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
    const nlohmann::json l_report_json = l_report;
    co_await l_kitsu_client->post_ai_light_render_report(task_info_.uuid_id_, l_report_json);
  } catch (const std::exception& l_ex) {
    SPDLOG_LOGGER_ERROR(l_logger, "回传渲染报告失败: {}", l_ex.what());
    if (l_error_msg.empty()) l_error_msg = l_ex.what();
  }

  // 本地产物**不删**（D43）：上传成功也不清 —— 出问题时它是唯一的现场，
  // 磁盘增长交给人工/后续定时清理。这里只把路径打进日志，方便事后去捡。
  if (!l_local_out_dir.empty())
    SPDLOG_LOGGER_INFO(
        l_logger, "AI 灯光第 {} 轮结束，本地产物目录（保留不删）：{}", payload_.iteration_, l_local_out_dir.generic_string()
    );

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
