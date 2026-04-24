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
// /api/user/seedance2/task
DOODLE_HTTP_JWT_FUN(user_seedance2_task)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// /api/seedance2/task
DOODLE_HTTP_JWT_FUN(seedance2_task)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()

// /api/seedance2/task/{id}
DOODLE_HTTP_JWT_FUN(seedance2_task_instance)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid id_{};
DOODLE_HTTP_FUN_END()

// /api/seedance2/thumbnail/task/{id}.png
DOODLE_HTTP_JWT_FUN(seedance2_thumbnail_task)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/seedance2/pictures/task/{id}.png
DOODLE_HTTP_JWT_FUN(seedance2_pictures_task)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()

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
// /api/seedance2/animation/waiting.mp4
DOODLE_HTTP_JWT_FUN(seedance2_animation_waiting)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()

}  // namespace doodle::http::seedance2