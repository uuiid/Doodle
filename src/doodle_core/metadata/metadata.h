//
// Created by teXiao on 2021/4/27.
//

#pragma once

#include "doodle_core/core/core_help_impl.h"
#include <doodle_core/doodle_core_fwd.h>

#include "boost/lexical_cast.hpp"
#include "boost/uuid/uuid.hpp"

#include <entt/core/type_info.hpp>
#include <optional>

namespace doodle {
class database;
namespace snapshot {
void load_com(database &in_entity, std::shared_ptr<void> &in_pre);
}

void DOODLE_CORE_API to_json(nlohmann::json &j, const database &p);
void DOODLE_CORE_API from_json(const nlohmann::json &j, database &p);

}  // namespace doodle
