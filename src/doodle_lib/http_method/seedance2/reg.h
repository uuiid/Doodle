//
// Created by TD on 25-4-1.
//

#pragma once
#include "doodle_core/doodle_core_fwd.h"
#include <doodle_core/metadata/ai_studio.h>
#include <doodle_core/metadata/seedance2/canvas_media.h>
#include <doodle_core/metadata/seedance2/task.h>

#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_client/ai_client_base.h>
#include <doodle_lib/http_method/http_jwt_fun.h>

#include <boost/asio/awaitable.hpp>
#include <core/http/http_function.h>
#include <memory>
#include <sqlite_orm/orm/update.h>

namespace doodle::http::seedance2 {
// 设置当周人员剩余可使用的 token 数量
orm::update_t add_remaining_tokens_for_person(
    sqlite_database& in_sql, const uuid& in_person_id, std::int64_t in_tokens
);
// 获取当周人员可以使用的 token 数量
std::int64_t get_remaining_tokens_for_person(const uuid& in_person_id);

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
std::shared_ptr<ai_client_base> make_inference_client(
    const ai_studio& in_studio, doodle::seedance2::task_backend in_backend = doodle::seedance2::task_backend::seedance2
);

/// 同步执行一次推理并把任务行落成终态(见上面的说明)。失败也会落库(便于审计与前台展示),
/// 但 completion_tokens_ 记 0(不计费); 原始响应始终保留在 data_response_ 里。
boost::asio::awaitable<ai_client_base::run_task_result_t> run_inference_task(
    const doodle::seedance2::task& in_task, sqlite_database& in_sql, const std::shared_ptr<ai_client_base>& in_client
);

// 任务相关 ------------------
// /api/seedance2/thumbnail/{id}.png
DOODLE_HTTP_JWT_FUN(seedance2_thumbnail)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/seedance2/pictures/{id}{file_extension}
// 这个路由要把 URL 交给第三方(方舟)取图, 所以除会话鉴权外还接受短时效签名 URL
DOODLE_HTTP_SIGNED_FUN(seedance2_pictures)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
file_extension_t file_extension_{};
DOODLE_HTTP_FUN_END()
// 等待动画
// /api/seedance2/animation/waiting.mp4
DOODLE_HTTP_JWT_FUN(seedance2_animation_waiting)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()

/// 子项目, 参与人员相关 ---------------------
// /api/seedance2/subproject
DOODLE_HTTP_JWT_FUN(seedance2_subproject)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_END()
// /api/seedance2/subproject/{id}
DOODLE_HTTP_JWT_FUN(seedance2_subproject_instance)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// 添加缩略图
// /api/seedance2/subproject/{id}/preview
DOODLE_HTTP_JWT_FUN(seedance2_subproject_preview)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid id_{};
DOODLE_HTTP_FUN_END()

// /api/seedance2/subproject/{subproject_id}/person
DOODLE_HTTP_JWT_FUN(seedance2_subproject_person_link)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid subproject_id_{};
DOODLE_HTTP_FUN_END()
// /api/seedance2/subproject/{subproject_id}/episodes
DOODLE_HTTP_JWT_FUN(seedance2_subproject_ai_episode)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid subproject_id_{};
DOODLE_HTTP_FUN_END()
// /api/seedance2/subproject/{subproject_id}/episodes/{episode_id}
DOODLE_HTTP_JWT_FUN(seedance2_subproject_ai_episode_instance)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid subproject_id_{};
uuid episode_id_{};
DOODLE_HTTP_FUN_END()
// /api/seedance2/subproject/{subproject_id}/episodes/{episode_id}/model-resolution-limit
DOODLE_HTTP_JWT_FUN(seedance2_subproject_ai_episode_model_resolution_limit)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid subproject_id_{};
uuid episode_id_{};
DOODLE_HTTP_FUN_END()
// /api/seedance2/subproject/{subproject_id}/model-resolution-limit/{limit_id}
DOODLE_HTTP_JWT_FUN(seedance2_subproject_model_resolution_limit_instance)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid subproject_id_{};
uuid episode_id_{};
uuid limit_id_{};
DOODLE_HTTP_FUN_END()
// /api/seedance2/subproject/{subproject_id}/entity
DOODLE_HTTP_JWT_FUN(seedance2_subproject_entity)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid subproject_id_{};
DOODLE_HTTP_FUN_END()
// /api/seedance2/subproject/{subproject_id}/episodes/{episode_id}/entity
DOODLE_HTTP_JWT_FUN(seedance2_subproject_ai_generate_entity)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(put)
uuid subproject_id_{};
uuid episode_id_{};
DOODLE_HTTP_FUN_END()
// /api/seedance2/subproject/{subproject_id}/entity/{entity_id}
DOODLE_HTTP_JWT_FUN(seedance2_subproject_ai_generate_entity_instance)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid subproject_id_{};
uuid entity_id_{};
DOODLE_HTTP_FUN_END()
// /api/seedance2/subproject/{subproject_id}/entity/{entity_id}/preview
DOODLE_HTTP_JWT_FUN(seedance2_subproject_entity_preview)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid subproject_id_{};
uuid entity_id_{};
DOODLE_HTTP_FUN_END()

// /api/seedance2/subproject/{subproject_id}/entity/{entity_id}/task
DOODLE_HTTP_JWT_FUN(seedance2_subproject_task)
seedance2_subproject_task();
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid subproject_id_{};
uuid entity_id_{};
DOODLE_HTTP_FUN_END()
// 获取所有可以访问的任务
// /api/seedance2/task
DOODLE_HTTP_JWT_FUN(seedance2_task)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// 任务运行管理器: 获取运行状态 / 启动运行循环
// /api/seedance2/task/run
DOODLE_HTTP_JWT_FUN(seedance2_task_run)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_END()
//
// /api/seedance2/task/{date}-{date}
DOODLE_HTTP_JWT_FUN(seedance2_task_date)
DOODLE_HTTP_FUN_OVERRIDE(get)
chrono::year_month_day date_start_{};
chrono::year_month_day date_end_{};
DOODLE_HTTP_FUN_END()
// /api/seedance2/subproject/{subproject_id}/task/{id}
DOODLE_HTTP_JWT_FUN(seedance2_subproject_task_instance)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid subproject_id_{};
uuid id_{};
DOODLE_HTTP_FUN_END()
/// 参考实现对应 ai_entity_reference_preview
// /api/seedance2/subproject/{subproject_id}/entity/{entity_id}/reference
DOODLE_HTTP_JWT_FUN(seedance2_subproject_entity_reference)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid subproject_id_{};
uuid entity_id_{};
DOODLE_HTTP_FUN_END()
// 删除 ai_entity_reference_preview
// /api/seedance2/subproject/{subproject_id}/reference/{id}
DOODLE_HTTP_JWT_FUN(seedance2_subproject_reference_instance)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid subproject_id_{};
uuid id_{};
DOODLE_HTTP_FUN_END()

/// AI 实体类别相关 ------------------------------
// /api/seedance2/subproject/{subproject_id}/category
DOODLE_HTTP_JWT_FUN(seedance2_subproject_ai_category)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid subproject_id_{};
DOODLE_HTTP_FUN_END()
// /api/seedance2/subproject/{subproject_id}/category/{category_id}
DOODLE_HTTP_JWT_FUN(seedance2_subproject_ai_category_instance)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid subproject_id_{};
uuid category_id_{};
DOODLE_HTTP_FUN_END()
// /api/seedance2/subproject/{subproject_id}/category/{category_id}/entity
DOODLE_HTTP_JWT_FUN(seedance2_subproject_ai_category_entity)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid subproject_id_{};
uuid category_id_{};
DOODLE_HTTP_FUN_END()

/// 限额相关 -----------------------------------
// /api/seedance2/tokens
DOODLE_HTTP_JWT_FUN(seedance2_tokens)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_END()
// /api/seedance2/tokens/person/{person_id}
DOODLE_HTTP_JWT_FUN(seedance2_tokens_person_instance)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(put)
uuid person_id_{};
DOODLE_HTTP_FUN_END()
// /api/seedance2/tokens/person/all
DOODLE_HTTP_JWT_FUN(seedance2_tokens_person_all)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// 获取某人某段时间 token 消耗统计
// /api/seedance2/tokens/person/{person_id}/date/{date}-{date}
DOODLE_HTTP_JWT_FUN(seedance2_tokens_person_date_instance)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid person_id_{};
chrono::year_month_day date_start_{};
chrono::year_month_day date_end_{};
DOODLE_HTTP_FUN_END()
// /api/seedance2/tokens/person/{person_id}/date/{date}
DOODLE_HTTP_JWT_FUN(seedance2_tokens_person_date_instance_day)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid person_id_{};
chrono::year_month_day date_{};
DOODLE_HTTP_FUN_END()

// 获取某段时间所有人 token 消耗统计
// /api/seedance2/tokens/person/date/{date}-{date}
DOODLE_HTTP_JWT_FUN(seedance2_tokens_person_date_all)
DOODLE_HTTP_FUN_OVERRIDE(get)
chrono::year_month_day date_start_{};
chrono::year_month_day date_end_{};
DOODLE_HTTP_FUN_END()
// 统计所有人, 某天的 token 消耗量
// /api/seedance2/tokens/person/date/{date}
DOODLE_HTTP_JWT_FUN(seedance2_tokens_person_date)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
chrono::year_month_day date_{};
DOODLE_HTTP_FUN_END()
// 获取预警信息(task重复度)
// /api/seedance2/task-similarity
DOODLE_HTTP_JWT_FUN(seedance2_task_similarity)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()

/// 画布相关 ---------------------
// /api/canvas
DOODLE_HTTP_JWT_FUN(canvas_)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_END()
// /api/canvas/{canvas_id}
DOODLE_HTTP_JWT_FUN(canvas_instance)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid canvas_id_{};
DOODLE_HTTP_FUN_END()

/// 画布节点 ---------------------
// /api/canvas/{canvas_id}/node
DOODLE_HTTP_JWT_FUN(canvas_node)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid canvas_id_{};
DOODLE_HTTP_FUN_END()
// /api/canvas/{canvas_id}/node/{node_id}
DOODLE_HTTP_JWT_FUN(canvas_node_instance)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid canvas_id_{};
uuid node_id_{};
DOODLE_HTTP_FUN_END()

/// 画布连线 ---------------------
// /api/canvas/{canvas_id}/connection
DOODLE_HTTP_JWT_FUN(canvas_connection)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid canvas_id_{};
DOODLE_HTTP_FUN_END()
// /api/canvas/{canvas_id}/connection/{connection_id}
DOODLE_HTTP_JWT_FUN(canvas_connection_instance)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid canvas_id_{};
uuid connection_id_{};
DOODLE_HTTP_FUN_END()

/// 画布媒体 ---------------------
// /api/canvas/{canvas_id}/node/{node_id}/media
DOODLE_HTTP_JWT_FUN(canvas_media)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid canvas_id_{};
uuid node_id_{};
DOODLE_HTTP_FUN_END()
// /api/canvas/{canvas_id}/node/{node_id}/media/{media_id}
DOODLE_HTTP_JWT_FUN(canvas_media_instance)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid canvas_id_{};
uuid node_id_{};
uuid media_id_{};
DOODLE_HTTP_FUN_END()
// /api/canvas/{canvas_id}/node/{node_id}/media/{media_id}/file/{role}
DOODLE_HTTP_JWT_FUN(canvas_media_file)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(put)
uuid canvas_id_{};
uuid node_id_{};
uuid media_id_{};
doodle::seedance2::media_role role_{};
DOODLE_HTTP_FUN_END()

// /api/seedance2/subproject/{subproject_id}/entity/{entity_id}/depth
DOODLE_HTTP_JWT_FUN(doodle_ai_depth_estimation_video)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid subproject_id_{};
uuid entity_id_{};
DOODLE_HTTP_FUN_END()

// /api/seedance2/depth/{depth_id}
DOODLE_HTTP_JWT_FUN(doodle_ai_depth_estimation_file)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(put)
uuid depth_id_;
DOODLE_HTTP_FUN_END()

}  // namespace doodle::http::seedance2