//
// Created by TD on 24-7-11.
//

#include "share_fun.h"

#include <doodle_core/metadata/user.h>

#include <entt/entt.hpp>
namespace doodle::http {
std::tuple<entt::handle, user> find_user_handle(entt::registry& reg, const boost::uuids::uuid& user_id) {
  auto l_v = reg.view<const user>();
  for (auto&& [e, l_user] : l_v.each()) {
    if (l_user.id_ == user_id) return {{reg, e}, l_user};
  }
  return {};
}
}  // namespace doodle::http