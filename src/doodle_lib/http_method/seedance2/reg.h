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
// /api/seedance2/task
// /api/seedance2/task/{id}
// /api/seedance2/thumbnail/task/{id}.png
// /api/seedance2/pictures/task/{id}.png

// /api/seedance2/asset-library/entity/{parent_id}/item
// /api/seedance2/asset-library/entity/{parent_id}/item/{id}


// /api/seedance2/asset-library/group/{group_id}/entity
// /api/seedance2/asset-library/entity/{entity_id}
// /api/seedance2/asset-library/entity/search

// /api/seedance2/asset-library/group
// /api/seedance2/asset-library/group/{group_id}

// /api/seedance2/asset-library/entity/{parent_id}/pictures/item/{id}.png
// /api/seedance2/asset-library/entity/{parent_id}/thumbnail/item/{id}.png

// /api/seedance2/animation/waiting.png
}