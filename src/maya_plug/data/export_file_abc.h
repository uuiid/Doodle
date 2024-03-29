//
// Created by td_main on 2023/4/27.
//

#pragma once
#include <maya_plug/maya_plug_fwd.h>

#include "entt/entity/fwd.hpp"
#include "maya/MApiNamespace.h"
#include <filesystem>
#include <maya/MTime.h>
#include <string>
#include <vector>

namespace doodle::maya_plug {
class reference_file;
class export_file_abc {
 private:
  std::vector<MDagPath> child_export_model(const MDagPath& in_root);
  std::vector<MDagPath> find_out_group_child_suffix_node(const MDagPath& in_root, const std::string& in_suffix);

 protected:
  std::string m_name{};
  MTime begin_time{};
  MTime end_time{};
  MSelectionList l_current_export_list{};
  virtual void export_abc(const MSelectionList& in_select, const FSys::path& in_path);

 public:
  export_file_abc() = default;
  std::vector<FSys::path> export_sim(const entt::handle_view<reference_file, generate_file_path_ptr>& in_handle);
  [[nodiscard]] inline MSelectionList get_export_list() const { return l_current_export_list; };
};

}  // namespace doodle::maya_plug
