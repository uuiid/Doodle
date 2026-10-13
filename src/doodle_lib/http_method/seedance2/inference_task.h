//
// 推理任务的对外入口：客户端工厂 + 同步执行并结算。
//
// 为什么单独一份头文件（而不是留在 reg.h 里）：
//   1. reg.h 是**路由注册**头，包含它会把一整套路由类拖进来；而这两个函数是给
//      推理网关（ai_light_gateway.cpp）与接缝共用管道（ai_light_seam_common.cpp）
//      用的，与路由无关；
//      （曾经的使用者还有接缝 generate_candidates / structure_feedback，
//        两条均已随「推理归前端」删除）
//   2. reg.h 里有 `#include <core/http/http_function.h>` 这种「相对 doodle_lib 根」的写法，
//      测试目标没有把 src/doodle_lib 放进 include 路径 —— 在测试里包含 reg.h 会直接编译失败。
//      把入口挪到这份自洽的头里，测试只需依赖它（seedance2_task_pipeline.cpp 就是这么做的）。
//

#pragma once

#include <doodle_core/metadata/ai_studio.h>
#include <doodle_core/metadata/seedance2/task.h>

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_client/ai_client_base.h>

#include <boost/asio/awaitable.hpp>

#include <memory>

namespace doodle::http::seedance2 {

// ─── 推理任务(火山方舟 Responses API) ────────────────────────────────────────
// 设计: 推理走「方案 A」—— 调用方(接缝)同步等待结果, 任务行**插入即终态**,
// 且插入与 token 扣减在同一个事务里, 所以后台的 seedance2_task_run_manager
// (5 秒轮询 preparing/queued/running) 永远看不到这条任务, 不会重复提交。
//
// 与生成任务的差别:
//   * 没有 result_files_/preview_file_(结果是一段文本, 落在 task.result_text_);
//   * 没有预扣/退补: 计费按响应的 total_tokens 当场结算(输入 token 占大头);
//   * 不落方舟侧(请求固定 store=false), 因此也没有 previous_response_id 的多轮。

/// 构造一个推理客户端。**每次调用都新建**: http_client_ssl 内部只有一个 socket 且无锁,
/// 共享给并发请求会串包。
/// in_backend 决定用哪个后端; 目前只有 seedance2 支持推理, transfer_station 会返回失败结果。
DOODLELIB_API std::shared_ptr<ai_client_base> make_inference_client(
    const ai_studio& in_studio, doodle::seedance2::task_backend in_backend = doodle::seedance2::task_backend::seedance2
);

/// 同步执行一次推理并把任务行落成终态(见上面的说明)。失败也会落库(便于审计与前台展示),
/// 但 completion_tokens_ 记 0(不计费); 原始响应始终保留在 data_response_ 里。
DOODLELIB_API boost::asio::awaitable<ai_client_base::run_task_result_t> run_inference_task(
    const doodle::seedance2::task& in_task, sqlite_database& in_sql, const std::shared_ptr<ai_client_base>& in_client
);

}  // namespace doodle::http::seedance2
