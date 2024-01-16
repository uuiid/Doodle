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

  MDGModifier dg_modifier_{};
  // 强制
  bool is_force{false};

  MObject p_current_node{};
  MSelectionList p_selection_list{};
};

}  // namespace doodle::maya_plug
