//
// Created by TD on 2021/5/7.
//

#pragma once
#include "doodle_core/configure/doodle_core_export.h"
#include "doodle_core/metadata/metadata.h"
#include <doodle_core/doodle_core_fwd.h>

#include <boost/uuid/uuid.hpp>

#include <entt/entity/fwd.hpp>
#include <string>

namespace doodle {

class DOODLE_CORE_API user_ref {
  mutable database::ref_data user_ref_attr{};

  mutable entt::handle handle_cache;
  friend void DOODLE_CORE_API to_json(nlohmann::json& j, const user_ref& p);
  friend void DOODLE_CORE_API from_json(const nlohmann::json& j, user_ref& p);

 public:
  user_ref() = default;
  explicit user_ref(const entt::handle& in_handle);
  mutable std::string cache_name;
  void set_uuid(const boost::uuids::uuid& in_data_uuid);
  [[nodiscard]] const boost::uuids::uuid& get_uuid() const;

  [[nodiscard]] entt::handle user_attr() const;
  // [[nodiscard]] entt::handle user_attr();
  void user_attr(const entt::handle& in_user);
};
}  // namespace doodle