//
// Created by td_main on 2023/4/27.
//

#pragma once
#include <maya_plug/maya_plug_fwd.h>

#include "entt/entity/fwd.hpp"
#include <vector>

namespace doodle::maya_plug {
class reference_file;
class export_file_abc {
 private:
  std::vector<MDagPath> cloth_export_model(const entt::handle_view<reference_file>& in_handle);

 public:
  export_file_abc() = default;
  void export_sim(const entt::handle_view<reference_file, generate_file_path_ptr>& in_handle);
};

}  // namespace doodle::maya_plug
