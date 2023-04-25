//
// Created by TD on 2022/7/29.
//
#pragma once
#include <maya_plug/main/maya_plug_fwd.h>
namespace doodle::maya_plug {

namespace sequence_to_blend_shape_comm_ns {
constexpr char name[] = "doodle_sequence_to_blend_shape";
MSyntax syntax();
}  // namespace sequence_to_blend_shape_comm_ns

class sequence_to_blend_shape_comm : public doodle::TemplateAction<
                                         sequence_to_blend_shape_comm, sequence_to_blend_shape_comm_ns::name,
                                         sequence_to_blend_shape_comm_ns::syntax> {
  class impl;
  std::unique_ptr<impl> p_i;
  void get_arg(const MArgList& in_arg);

  void delete_node();
  void create_mesh();
  void create_anim();
  void run_blend_shape_comm();
  void add_to_parent();
  void run_pca();

 public:
  sequence_to_blend_shape_comm();
  ~sequence_to_blend_shape_comm() override;

  MStatus doIt(const MArgList& in_arg) override;
  [[maybe_unused]] MStatus undoIt() override;
  [[maybe_unused]] MStatus redoIt() override;
  [[maybe_unused]] [[nodiscard]] bool isUndoable() const override;
};

}  // namespace doodle::maya_plug
