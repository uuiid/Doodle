//
// Created by td_main on 2023/7/6.
//
#pragma once
#include <doodle_core/core/app_base.h>

#include <maya_plug/data/reference_file.h>

#include "entt/entity/fwd.hpp"
#include "exe_maya/core/maya_lib_guard.h"
#include "maya/MApiNamespace.h"
#include <cstdint>
#include <maya/MTime.h>
#include <memory>
#include <string>
#include <vector>
namespace doodle::maya_plug {

class replace_file_facet {
  static constexpr auto config{"replace_file_config"};
  void replace_file(const std::vector<std::pair<FSys::path, FSys::path>>& in_files, const FSys::path& in_file_path);

  std::vector<reference_file> ref_files_{};

 public:
  replace_file_facet()  = default;
  ~replace_file_facet() = default;

  bool post(const nlohmann::json& in_argh);
};

}  // namespace doodle::maya_plug
