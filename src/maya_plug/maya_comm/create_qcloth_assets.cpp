//
// Created by TD on 2022/7/20.
//

#include "create_qcloth_assets.h"

namespace doodle::maya_plug {
namespace create_qcloth_assets_ns {

char cloth[]       = {"-c"};
char cloth_l[]     = {"-cloth"};

char high[]        = {"-h"};
char high_l[]      = {"-high"};

char collision[]   = {"-co"};
char collision_l[] = {"-collision"};

MSyntax syntax() {
  MSyntax l_syntax{};
  l_syntax.addFlag(cloth, cloth_l, MSyntax::MArgType::kString);
  l_syntax.addFlag(high, high_l, MSyntax::MArgType::kString);
  l_syntax.addFlag(collision, collision_l, MSyntax::MArgType::kString);

  l_syntax.makeFlagMultiUse(cloth);
  l_syntax.makeFlagMultiUse(high);
  l_syntax.makeFlagMultiUse(collision);
  return l_syntax;
}
}  // namespace create_qcloth_assets_ns

class create_qcloth_assets::impl {
 public:
  



};

create_qcloth_assets::create_qcloth_assets()
    : p_i(std::make_unique<impl>()) {
}
void create_qcloth_assets::parse_arg(const MArgList& in_arg) {
}
MStatus create_qcloth_assets::doIt(const MArgList& in_arg) {
  return redoIt();
}
MStatus create_qcloth_assets::undoIt() {
  return MStatus();
}
MStatus create_qcloth_assets::redoIt() {
  return MStatus();
}
bool create_qcloth_assets::isUndoable() const {
  return true;
}

create_qcloth_assets::~create_qcloth_assets() = default;
}  // namespace doodle::maya_plug
