//
// Created by TD on 2021/5/7.
//

#pragma once
#include "configure/doodle_core_export.h"
#include "doodle_core/metadata/metadata.h"
#include <doodle_core/doodle_core_fwd.h>

#include <boost/uuid/uuid.hpp>

#include <entt/entity/fwd.hpp>
#include <string>

namespace doodle {

class DOODLE_CORE_API user_ref {
  database::ref_data user_ref_attr{};

  entt::handle handle_cache;

 public:
  std::string cache_name;
  void set_uuid(const boost::uuids::uuid& in_data_uuid);
  [[nodiscard]] const boost::uuids::uuid& get_uuid() const;

  [[nodiscard]] entt::handle user_attr() const;
  [[nodiscard]] entt::handle user_attr();
  void user_attr(const entt::handle& in_user);
};
}  // namespace doodle