//
// Created by TD on 2022/7/29.
//
#pragma once
#include <maya_plug/maya_plug_fwd.h>
namespace doodle {
namespace maya_plug {

namespace sequence_to_blend_shape_ns {
constexpr char name[] = "doodle_sequence_to_blend_shape";
MSyntax syntax();
}  // namespace sequence_to_blend_shape_ns

class sequence_to_blend_shape : public doodle::TemplateAction<
                                    sequence_to_blend_shape,
                                    sequence_to_blend_shape_ns::name,
                                    sequence_to_blend_shape_ns::syntax> {
  class impl;
  std::unique_ptr<impl> p_i;
  void get_arg(const MArgList& in_arg);
 public:
  sequence_to_blend_shape();
  ~sequence_to_blend_shape() override;

  MStatus doIt(const MArgList& in_arg) override;
};

}  // namespace maya_plug
}  // namespace doodle
