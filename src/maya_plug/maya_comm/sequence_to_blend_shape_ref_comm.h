//
// Created by TD on 2022/8/15.
//
#pragma once

#include <maya_plug/main/maya_plug_fwd.h>

namespace doodle {
namespace maya_plug {

namespace sequence_to_blend_shape_ref_comm_ns {
constexpr char name[] = "doodle_sequence_to_blend_shape_ref";
MSyntax syntax();
}  // namespace sequence_to_blend_shape_ref_comm_ns

class sequence_to_blend_shape_ref_comm : public doodle::TemplateAction<
                                             sequence_to_blend_shape_ref_comm,
                                             sequence_to_blend_shape_ref_comm_ns::name,
                                             sequence_to_blend_shape_ref_comm_ns::syntax> {
  class impl;
  std::unique_ptr<impl> p_i;
  void get_arg(const MArgList& in_arg);

  void create_mesh();
  void create_anim();
  void run_blend_shape_comm();
  void add_to_parent();
  void delete_node();
 public:
  sequence_to_blend_shape_ref_comm();
  ~sequence_to_blend_shape_ref_comm() override;

  MStatus doIt(const MArgList& in_arg) override;
  [[maybe_unused]] MStatus undoIt() override;
  [[maybe_unused]] MStatus redoIt() override;
  [[maybe_unused]] [[nodiscard]] bool isUndoable() const override;
};

}  // namespace maya_plug
}  // namespace doodle
