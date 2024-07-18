//
// Created by TD on 24-7-11.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

namespace doodle::http {
std::tuple<entt::handle, user> find_user_handle(entt::registry& reg, const boost::uuids::uuid& user_id);
}