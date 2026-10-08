//
// AI 灯光分布式任务：一轮候选的渲染（工作机侧）
//
// 服务端把整轮载荷塞进 server_task_info.command_（ai_light::ai_light_task_payload_t），
// 这里**只起一次 UE**（不带 -run=，进程靠插件模块的 UDoodleLightingRoundRunner 驱动，D46），
// UE 侧用 auto_light 已经生成好的关卡序列 + 电影管线配置在 PIE 世界里逐候选走 MRQ 渲染出一帧帧 PNG；
// 回到这里再把帧合成 <候选 uuid>.mp4、把首帧复制成 <候选 uuid>.png 当海报，两个都 HTTP 上传回服务端
// （POST /api/doodle/ai/light/candidate/{id}/file/{render|render_movie}），
// 最后把 <out_dir>/render_report.json 回传给 POST /api/doodle/ai/light/render_report/{task_id}。
//
// 产物落**工作机本地**（D38/D39）：载荷里只有相对路径，本地根由本机定，不写服务端的
// 网络盘/UNC 目录；UE 侧只认载荷给的绝对路径，是这里在起 UE 之前把相对路径拼成本地绝对路径。
//
// 职责边界：
//   - 灯怎么打、**服务端**的产物落点怎么算，全在服务端 —— 这里只搬运、合成与执行，
//     本地怎么落由本机定；
//   - 单个候选渲染/合成/上传失败不该让整轮失败（方案 §7.5 第 9 条）：这里把该候选降级成
//     failed 再如实回传，由服务端决定「算不算推进」；
//   - 上传顺序有约束：海报与视频必须在回传报告**之前**传完，服务端 D41 就是靠查这两个文件
//     在不在来判「候选是否真的可判断」；
//   - 合成帧率取 UE 回传的 frame_rate_（= 序列 display rate），拿不到才退回 25；
//   - 本地产物不删（D43），只在日志里报路径；
//   - 任务的终态（completed / failed）仍按分布式契约单独上报（put_job_info）——
//     会话状态与任务状态是两层，别混。
//

#pragma once

#include <doodle_core/metadata/ai_light/task_payload.h>
#include <doodle_core/metadata/server_task_info.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_client/work.h>

#include <boost/asio/awaitable.hpp>

#include <memory>

namespace doodle::http {

/// 一轮 AI 灯光渲染。继承 base_distributed_task —— 日志回传、任务计数、终态上报都由它提供。
class DOODLELIB_API ai_light_round_distributed : public base_distributed_task {
  ai_light::ai_light_task_payload_t payload_{};

 public:
  explicit ai_light_round_distributed(server_task_info in_task_info, std::shared_ptr<http_work> in_http_work_ptr)
      : base_distributed_task(std::move(in_task_info), std::move(in_http_work_ptr)) {}
  ~ai_light_round_distributed() override = default;

  boost::asio::awaitable<void> run();
};

}  // namespace doodle::http
