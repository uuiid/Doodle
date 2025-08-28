//
// Created by TD on 25-8-28.
//

#include <maya_plug/maya_plug_fwd.h>

namespace doodle::maya_plug {
constexpr char name_doodle_head_weight[] = "doodle_head_weight";
class head_weight : public TemplateAction<head_weight, name_doodle_head_weight, null_syntax_t> {
public:
  MStatus doIt(const MArgList& args) override;
};
}  // namespace doodle::maya_plug