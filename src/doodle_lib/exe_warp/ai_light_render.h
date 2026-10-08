//
// AI 灯光分布式任务：一轮候选的渲染（工作机侧，方案 §15.12 / §15.13 批 6）
//
// 服务端把整轮载荷塞进 server_task_info.command_（ai_light::ai_light_task_payload_t），
// 这里**只起一次 UE**（DoodleLighting commandlet）渲染全部候选，然后把产物 HTTP 上传回
// 服务端（POST /api/doodle/ai/light/candidate/{id}/file/{render|render_linear}），
// 最后把 <out_dir>/render_report.json 回传给 POST /api/doodle/ai/light/render_report/{task_id}。
//
// 批 6 起产物落**工作机本地**（D38/D39：载荷里只有相对路径，本地根由本机定），
// 不再直写服务端推广出来的网络盘/UNC 目录；UE 侧因此一行都不用改 —— 它本来就只认
// 载荷给的 PngPath/ExrPath，是这里在起 UE 之前把相对路径拼成了本地绝对路径。
//
// 职责边界：
//   - 灯怎么打、**服务端**的落库路径怎么算，全在服务端 —— 这里只搬运与执行，
//     本地怎么落由本机定；
//   - 单张候选渲染失败不该让整轮失败（方案 §7.5 第 9 条）：commandlet 逐候选写
//     status/error，这里原样回传（只补一条「PNG 传不上去就等于没渲染成功」），
//     由服务端决定「算不算推进」；
//   - 上传顺序有约束：PNG 必须在回传报告**之前**传完，服务端 D41 就是靠查那张文件
//     在不在来判「候选是否真的可判断」；
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
