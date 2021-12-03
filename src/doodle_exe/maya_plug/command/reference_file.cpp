//
// Created by TD on 2021/11/30.
//

#include "reference_file.h"

#include <doodle_lib/metadata/metadata.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MItSelectionList.h>
#include <maya/MString.h>
#include <maya/MUuid.h>
#include <maya_plug/maya_plug_fwd.h>
namespace doodle::maya_plug {
reference_file::reference_file()
    : prj_ref(boost::uuids::nil_uuid()),
      path(),
      use_sim(false),
      high_speed_sim(false),
      collision_model(){};

reference_file::reference_file(const entt::handle &in_uuid, const string &in_u8_path)
    : reference_file() {
  chick_component<database>(in_uuid);
  prj_ref = in_uuid.get<database>().uuid();
  path    = in_u8_path;
}
MSelectionList reference_file::get_collision_model() const {
  MSelectionList l_list{};
  for (const auto &str : collision_model) {
    l_list.add(str.c_str(), true);
  }
  return l_list;
}
void reference_file::set_collision_model(const MSelectionList &in_list) {
  collision_model.clear();
  collision_model_show_str.clear();
  MStatus k_s{};
  MDagPath l_path{};
  MFnDagNode l_node{};
  for (MItSelectionList l_it{in_list, MFn::Type::kMesh, &k_s};
       !l_it.isDone(&k_s);
       l_it.next()) {
    DOODLE_CHICK(k_s);
    k_s = l_it.getDagPath(l_path);
    DOODLE_CHICK(k_s);
    auto k_obj = l_path.transform(&k_s);
    DOODLE_CHICK(k_s);
    k_s = l_node.setObject(k_obj);
    DOODLE_CHICK(k_s);
    collision_model_show_str.emplace_back(l_node.name(&k_s).asUTF8());
    DOODLE_CHICK(k_s);

    collision_model.push_back(l_node.fullPathName(&k_s).asUTF8());
    DOODLE_CHICK(k_s);
  }
}

void reference_file::init_show_name() {
  collision_model_show_str.clear();
  MStatus k_s{};
  MDagPath l_path{};
  MFnDagNode l_node{};
  for (MItSelectionList l_it{get_collision_model(), MFn::Type::kMesh, &k_s};
       !l_it.isDone(&k_s);
       l_it.next()) {
    DOODLE_CHICK(k_s);
    k_s = l_it.getDagPath(l_path);
    DOODLE_CHICK(k_s);
    auto k_obj = l_path.transform(&k_s);
    DOODLE_CHICK(k_s);
    k_s = l_node.setObject(k_obj);
    DOODLE_CHICK(k_s);
    collision_model_show_str.emplace_back(l_node.name(&k_s).asUTF8());
    DOODLE_CHICK(k_s);
  }
}

}  // namespace doodle::maya_plug
