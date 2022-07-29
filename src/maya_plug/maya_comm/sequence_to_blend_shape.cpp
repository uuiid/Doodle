//
// Created by TD on 2022/7/29.
//

#include "sequence_to_blend_shape.h"

namespace doodle {
namespace maya_plug {

constexpr char startFrame_f[]  = "-sf";
constexpr char startFrame_lf[] = "-startFrame";
constexpr char endFrame_f[]    = "-ef";
constexpr char endFrame_lf[]   = "-endFrame";

constexpr char bindFrame_f[]   = "-bf";
constexpr char bindFrame_lf[]  = "-bindFrame";

constexpr char parent_f[]      = "-p";
constexpr char parent_lf[]     = "-parent";
namespace sequence_to_blend_shape_ns {
MSyntax syntax() {
  MSyntax syntax{};
  syntax.addFlag(startFrame_f, startFrame_lf, MSyntax::kTime);
  syntax.addFlag(endFrame_f, endFrame_lf, MSyntax::kTime);
  syntax.addFlag(bindFrame_f, bindFrame_lf, MSyntax::kTime);
  syntax.addFlag(parent_f, parent_lf, MSyntax::kString);
  return syntax;
}

}  // namespace sequence_to_blend_shape_ns

class sequence_to_blend_shape::impl {
 public:
};

sequence_to_blend_shape::sequence_to_blend_shape()
    : p_i(std::make_unique<impl>()) {
}

MStatus sequence_to_blend_shape::doIt(const MArgList& in_arg) {
  return MStatus();
}
void sequence_to_blend_shape::get_arg(const MArgList& in_arg) {
}

sequence_to_blend_shape::~sequence_to_blend_shape() = default;

}  // namespace maya_plug
}  // namespace doodle
