//
// Created by TD on 2022/10/17.
//

#pragma once
#include <maya_plug/main/maya_plug_fwd.h>

namespace doodle::maya_plug {
namespace add_entt_ns {
constexpr char name[] = "doodle_add_entt";
MSyntax syntax();
}  // namespace add_entt_ns
class add_entt : public TemplateAction<add_entt, add_entt_ns::name, add_entt_ns::syntax> {
 public:
  MStatus doIt(const MArgList& in_arg_list) override;
};

}  // namespace doodle::maya_plug
