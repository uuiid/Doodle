//
// Created by TD on 2022/7/20.
//

#pragma once
#include <maya_plug_fwd.h>
namespace doodle {
namespace maya_plug {
namespace create_qcloth_assets_ns {
constexpr char name[] = "doodle_create_qcloth_assets";
MSyntax syntax();
}  // namespace create_qcloth_assets_ns
class create_qcloth_assets : public TemplateAction<create_qcloth_assets,
                                                   create_qcloth_assets_ns::name,
                                                   create_qcloth_assets_ns::syntax> {
 public:
  [[maybe_unused]] MStatus doIt(const MArgList& in_arg) override;
  [[maybe_unused]] MStatus undoIt() override;
  [[maybe_unused]] MStatus redoIt() override;
  [[maybe_unused]] bool isUndoable() const override;
};

}  // namespace maya_plug
}  // namespace doodle
