//
// Created by td_main on 2023/9/26.
//

#include "doodle_to_ue_fbx.h"

#include <>

namespace doodle {
namespace maya_plug {

namespace {
constexpr char file_path[]   = "-file_path";
constexpr char file_path_l[] = "-fp";
}  // namespace

MSyntax doodle_to_ue_fbx_syntax() {
  MSyntax l_syntax{};
  l_syntax.addFlag(file_path, file_path_l, MSyntax::kString);
  return l_syntax;
}

doodle_to_ue_fbx::doodle_to_ue_fbx() = default;

MStatus doodle_to_ue_fbx::doIt(const MArgList &in_list) {}

doodle_to_ue_fbx::~doodle_to_ue_fbx() = default;

}  // namespace maya_plug
}  // namespace doodle