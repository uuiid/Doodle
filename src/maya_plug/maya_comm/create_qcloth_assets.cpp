//
// Created by TD on 2022/7/20.
//

#include "create_qcloth_assets.h"

#include <maya_plug/data/qcloth_shape.h>

#include <maya/MArgDatabase.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MItDag.h>
#include <maya/MArgList.h>
#include <maya/MDGModifier.h>

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
  l_syntax.enableQuery(false);
  l_syntax.enableEdit(false);
  return l_syntax;
}
}  // namespace create_qcloth_assets_ns

class create_qcloth_assets::impl {
 public:
  std::vector<entt::handle> cloth_list{};
  entt::handle coll_p;

  std::vector<MObject> create_nodes{};
};

create_qcloth_assets::create_qcloth_assets()
    : p_i(std::make_unique<impl>()) {
}
void create_qcloth_assets::parse_arg(const MArgList& in_arg) {
  DOODLE_LOG_INFO(in_arg);
  MStatus l_s{};
  MArgDatabase l_arg{syntax(), in_arg, &l_s};
  if (l_arg.isFlagSet(create_qcloth_assets_ns::cloth, &l_s)) {
    DOODLE_MAYA_CHICK(l_s);
    auto l_num = l_arg.numberOfFlagUses(create_qcloth_assets_ns::cloth);
    for (auto i = 0; i < l_num; ++i) {
      MArgList l_arg_list{};
      l_s = l_arg.getFlagArgumentList(create_qcloth_assets_ns::cloth, i, l_arg_list);
      DOODLE_MAYA_CHICK(l_s);

      p_i->cloth_list.emplace_back(make_handle(num_to_enum<entt::entity>(l_arg_list.asInt(0))));
    }
  }

  if (l_arg.isFlagSet(create_qcloth_assets_ns::collision, &l_s)) {
    DOODLE_MAYA_CHICK(l_s);
    std::int32_t l_value{};
    l_s = l_arg.getFlagArgument(create_qcloth_assets_ns::collision, 0, l_value);
    DOODLE_MAYA_CHICK(l_s);
    p_i->coll_p = make_handle(num_to_enum<entt::entity>(l_value));
  }

  p_i->cloth_list |= ranges::actions::remove_if([](const entt::handle& in) { return !in; });
  DOODLE_CHICK(!p_i->cloth_list.empty(), doodle_error{"传入了空的布料列表"s});
  if (!p_i->coll_p.valid())
    p_i->coll_p = {};
}
MStatus create_qcloth_assets::doIt(const MArgList& in_arg) {
  parse_arg(in_arg);
  return redoIt();
}
MStatus create_qcloth_assets::undoIt() {
  // 删除所有的创建成功的层级
  delete_node();
  // 更新所有的属性
  reset_properties();
  if (g_reg()->ctx().contains<qcloth_shape::cloth_group>()) {
    g_reg()->ctx().erase<qcloth_shape::cloth_group>();
  }
  return MStatus::kSuccess;
}
MStatus create_qcloth_assets::redoIt() {
  auto l_org_list = get_all_node();
  try {
    for (auto& l_h : p_i->cloth_list) {
      qcloth_shape::create_sim_cloth(l_h);
    }
    if (p_i->coll_p.any_of<qcloth_shape_n::shape_list>() && !p_i->cloth_list.empty())
      qcloth_shape::add_collider(p_i->coll_p);
    qcloth_shape::sort_group();
  } catch (const std::runtime_error& in_err) {
    filter_create_node(l_org_list);
    delete_node();
    return {MStatus::kFailure};
  }
  filter_create_node(l_org_list);
  return MStatus::kSuccess;
}
bool create_qcloth_assets::isUndoable() const {
  return true;
}
std::vector<MObject> create_qcloth_assets::get_all_node() {
  std::vector<MObject> l_r{};
  MStatus l_s{};
  for (MItDag l_it{};
       !l_it.isDone();
       l_it.next()) {
    l_r.emplace_back(l_it.currentItem(&l_s));
    DOODLE_MAYA_CHICK(l_s);
  }
  return l_r;
}
void create_qcloth_assets::delete_node() {
  //  MDGModifier l_modifier{};
  //  for (auto&& i : p_i->create_nodes) {
  //    if (!i.isNull())
  //      CHECK_MSTATUS(l_modifier.deleteNode(i));
  //  }
  //  DOODLE_MAYA_CHICK(l_modifier.doIt());

  MGlobal::deleteNode(g_reg()->ctx().at<qcloth_shape::cloth_group>().cfx_grp);
}
void create_qcloth_assets::filter_create_node(
    const std::vector<MObject>& in_obj
) {
  //  p_i->create_nodes = get_all_node();
  //  p_i->create_nodes |= ranges::actions::remove_if([&](const MObject& in) -> bool {
  //    auto it = ranges::find_if(in_obj, [&](const MObject& in_item) -> bool {
  //      return in == in_item;
  //    });
  //    return it != in_obj.end();
  //  });
  p_i->create_nodes.emplace_back(g_reg()->ctx().at<qcloth_shape::cloth_group>().cfx_grp);
}
void create_qcloth_assets::reset_properties() {
  for (auto& l_h : p_i->cloth_list) {
    qcloth_shape::reset_create_node_attribute(l_h);
  }
}

create_qcloth_assets::~create_qcloth_assets() = default;
}  // namespace doodle::maya_plug
