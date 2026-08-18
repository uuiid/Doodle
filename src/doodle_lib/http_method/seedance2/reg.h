//
// Created by TD on 25-4-1.
//

#pragma once
#include "doodle_core/doodle_core_fwd.h"

#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_method/http_jwt_fun.h>

#include <core/http/http_function.h>

namespace doodle::http::seedance2 {
// 设置当周人员剩余可使用的 token 数量
boost::asio::awaitable<void> add_remaining_tokens_for_person(const uuid& in_person_id, std::int64_t in_tokens);
// 获取当周人员可以使用的 token 数量
std::int64_t get_remaining_tokens_for_person(const uuid& in_person_id);

// 任务相关 ------------------
// /api/seedance2/thumbnail/{id}.png
DOODLE_HTTP_JWT_FUN(seedance2_thumbnail)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/seedance2/pictures/{id}.png
DOODLE_HTTP_JWT_FUN(seedance2_pictures)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
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
// /api/seedance2/subproject/{subproject_id}/classification
DOODLE_HTTP_JWT_FUN(seedance2_subproject_ai_generate_classification)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid subproject_id_{};
DOODLE_HTTP_FUN_END()
// /api/seedance2/subproject/{subproject_id}/classification/{classification_id}
DOODLE_HTTP_JWT_FUN(seedance2_subproject_ai_generate_classification_instance)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid subproject_id_{};
uuid classification_id_{};
DOODLE_HTTP_FUN_END()
// /api/seedance2/subproject/{subproject_id}/entity
DOODLE_HTTP_JWT_FUN(seedance2_subproject_ai_generate_entity)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid subproject_id_{};
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
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid subproject_id_{};
uuid entity_id_{};
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


/// 资产库相关 ---------------------
// /api/seedance2/asset-library/entity/{parent_id}/item
DOODLE_HTTP_JWT_FUN(seedance2_asset_library_entity_item)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid parent_id_{};
DOODLE_HTTP_FUN_END()
// /api/seedance2/asset-library/entity/{parent_id}/item/{id}
DOODLE_HTTP_JWT_FUN(seedance2_asset_library_entity_item_instance)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid parent_id_{};
uuid id_{};
DOODLE_HTTP_FUN_END()

// /api/seedance2/asset-library/group/{group_id}/entity
DOODLE_HTTP_JWT_FUN(seedance2_asset_library_group_entity)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid group_id_{};
DOODLE_HTTP_FUN_END()

// /api/seedance2/asset-library/entity/{entity_id}
DOODLE_HTTP_JWT_FUN(seedance2_asset_library_entity_instance)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid entity_id_{};
DOODLE_HTTP_FUN_END()

// /api/seedance2/asset-library/entity/search
DOODLE_HTTP_JWT_FUN(seedance2_asset_library_entity_search)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()

// /api/seedance2/asset-library/group
DOODLE_HTTP_JWT_FUN(seedance2_asset_library_group)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_END()
// /api/seedance2/asset-library/group/{group_id}
DOODLE_HTTP_JWT_FUN(seedance2_asset_library_group_instance)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid group_id_{};
DOODLE_HTTP_FUN_END()

// /api/seedance2/asset-library/entity/{parent_id}/pictures/item/{id}.png
DOODLE_HTTP_JWT_FUN(seedance2_asset_library_entity_pictures_item)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid parent_id_{};
uuid id_{};
DOODLE_HTTP_FUN_END()

// /api/seedance2/asset-library/entity/{parent_id}/thumbnail/item/{id}.png
DOODLE_HTTP_JWT_FUN(seedance2_asset_library_entity_thumbnail_item)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid parent_id_{};
uuid id_{};
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
}  // namespace doodle::http::seedance2