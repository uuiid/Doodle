//
// Created by td_main on 2023/4/27.
//

#pragma once
#include "entt/entity/fwd.hpp"
namespace doodle::maya_plug {
class reference_file;
class export_file_abc {
 public:
  export_file_abc() = default;
  void export_sim(const entt::handle_view<reference_file>& in_handle);
};

}  // namespace doodle::maya_plug
