//
// Created by TD on 2024/1/11.
//

#pragma once
#include <maya_plug/main/maya_plug_fwd.h>

#include "maya/MApiNamespace.h"
#include "maya/MDGModifier.h"
#include <maya/MSelectionList.h>
namespace doodle::maya_plug {

namespace file_info_edit_ns {
constexpr char file_info_edit[]{"doodle_file_info_edit"};
}  // namespace file_info_edit_ns

MSyntax file_info_edit_syntax();
class file_info_edit : public TemplateAction<file_info_edit, file_info_edit_ns::file_info_edit, file_info_edit_syntax> {
 public:
  file_info_edit();
  ~file_info_edit() override;
  [[maybe_unused]] MStatus doIt(const MArgList& in_list) override;
  [[maybe_unused]] MStatus undoIt() override;
  [[maybe_unused]] MStatus redoIt() override;
  [[maybe_unused]] [[nodiscard]] bool isUndoable() const override;

 private:
  MStatus delete_node();
  bool has_node() const;

  MStatus create_node();
  MStatus add_collision();
  MStatus add_wind_field();

  MStatus set_node_attr();
  MStatus override_node_attr();

  MDGModifier dg_modifier_{};
  // 强制
  bool is_force{false};
  // 忽略引用加载数据
  bool is_ignore_ref{false};

  MObject p_current_node{};
  MSelectionList p_selection_list{};

  using run_func_ptr = MStatus (file_info_edit::*)();
  run_func_ptr p_run_func{nullptr};

  std::optional<bool> sim_override;
  std::optional<bool> simple_subsampling;
  std::optional<std::int32_t> frame_samples;
  std::optional<std::double_t> time_scale;
  std::optional<std::double_t> length_scale;
  std::optional<std::int32_t> max_cg_iteration;
  std::optional<std::int32_t> cg_accuracy;
  std::optional<std::array<std::double_t, 3>> gravity;
  std::optional<bool> is_solve;
};

}  // namespace doodle::maya_plug
