//
// Created by TD on 2022/7/20.
//

#include "create_qcloth_assets.h"

#include <maya_plug/data/qcloth_shape.h>

#include <maya/MArgDatabase.h>

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
  l_syntax.addFlag(cloth, cloth_l, MSyntax::MArgType::kLong);
  l_syntax.addFlag(collision, collision_l, MSyntax::MArgType::kLong);

  l_syntax.makeFlagMultiUse(cloth);
  return l_syntax;
}
}  // namespace create_qcloth_assets_ns

class create_qcloth_assets::impl {
 public:
  std::vector<entt::handle> cloth_list{};
  entt::handle coll_p;
};

create_qcloth_assets::create_qcloth_assets()
    : p_i(std::make_unique<impl>()) {
}
void create_qcloth_assets::parse_arg(const MArgList& in_arg) {
  MStatus l_s{};
  MArgDatabase l_arg{syntax(), in_arg, &l_s};
  if (l_arg.isFlagSet(create_qcloth_assets_ns::cloth, &l_s)) {
    DOODLE_CHICK(l_s);
    for (auto i = 0; i < l_arg.numberOfFlagUses(create_qcloth_assets_ns::cloth); ++i) {
      std::int32_t l_value{};
      l_s = l_arg.getFlagArgument(create_qcloth_assets_ns::cloth, i, l_value);
      DOODLE_CHICK(l_s);
      p_i->cloth_list.emplace_back(make_handle(num_to_enum<entt::entity>(l_value)));
    }
  }

  if (l_arg.isFlagSet(create_qcloth_assets_ns::collision, &l_s)) {
    DOODLE_CHICK(l_s);
    std::int32_t l_value{};
    l_s = l_arg.getFlagArgument(create_qcloth_assets_ns::collision, 0, l_value);
    DOODLE_CHICK(l_s);
    p_i->coll_p = make_handle(num_to_enum<entt::entity>(l_value));
  }

  p_i->cloth_list |= ranges::action::remove_if([](const entt::handle& in) {
    return !in;
  });
  chick_true<doodle_error>(!p_i->cloth_list.empty(), DOODLE_LOC, "传入了空的布料列表");
  if (!p_i->coll_p.valid())
    p_i->coll_p = {};
}
MStatus create_qcloth_assets::doIt(const MArgList& in_arg) {
  parse_arg(in_arg);
  return redoIt();
}
MStatus create_qcloth_assets::undoIt() {
  DOODLE_LOG_WARN("create_qcloth_assets::undoIt()")
  return MStatus::kSuccess;
}
MStatus create_qcloth_assets::redoIt() {
  for (auto& l_h : p_i->cloth_list) {
    qcloth_shape::create_sim_cloth(l_h);
  }
  if (p_i->coll_p.any_of<qcloth_shape_n::shape_list>() && !p_i->cloth_list.empty())
    qcloth_shape::add_collider(p_i->coll_p);
  qcloth_shape::sort_group();
  return MStatus::kSuccess;
}
bool create_qcloth_assets::isUndoable() const {
  return true;
}

create_qcloth_assets::~create_qcloth_assets() = default;
}  // namespace doodle::maya_plug
