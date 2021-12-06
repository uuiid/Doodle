//
// Created by TD on 2021/11/30.
//

#include "reference_file.h"

#include <doodle_lib/metadata/metadata.h>
#include <doodle_lib/metadata/project.h>
#include <maya/MDagPath.h>
#include <maya/MFileIO.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnReference.h>
#include <maya/MItDependencyNodes.h>
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
      collision_model(),
      ref_file_uuid(),
      p_m_object(){};

reference_file::reference_file(const entt::handle &in_uuid, const MObject &in_ref_node)
    : reference_file() {
  chick_component<database>(in_uuid);
  prj_ref = in_uuid.get<database>().uuid();
  MStatus k_s{};
  MFnReference k_ref{in_ref_node, &k_s};
  DOODLE_CHICK(k_s);
  auto k_m_str = k_ref.fileName(true, true, true, &k_s);
  DOODLE_CHICK(k_s);
  path          = k_m_str.asUTF8();
  ref_file_uuid = k_ref.uuid().asString().asUTF8();
  p_m_object    = in_ref_node;
}

MSelectionList reference_file::get_collision_model() const {
  MSelectionList l_list{};
  for (const auto &str : collision_model) {
    l_list.add(str.c_str(), true);
  }
  return l_list;
}

void reference_file::chick_mobject() {
  if (p_m_object.isNull() && !ref_file_uuid.empty()) {
    MStatus k_s;
    MFnReference k_file;
    for (MItDependencyNodes refIter(MFn::kReference); !refIter.isDone(); refIter.next()) {
      k_s = k_file.setObject(refIter.thisNode());
      DOODLE_CHICK(k_s);
      if (k_file.uuid().asString().asUTF8() == ref_file_uuid) {
        p_m_object = refIter.thisNode();
      }
    }
    if (p_m_object.isNull())
      throw doodle_error{"无法找到引用"};
  }
}
void reference_file::chick_mobject() const {
  if (p_m_object.isNull()) {
    throw doodle_error{"没有初始化mobject"};
  }
}
void reference_file::set_collision_model(const MSelectionList &in_list) {
  collision_model.clear();
  collision_model_show_str.clear();
  chick_mobject();
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

    collision_model.emplace_back(l_node.fullPathName(&k_s).asUTF8());
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
string reference_file::get_namespace() const {
  chick_mobject();
  MFnReference k_ref{p_m_object};
  MStatus k_s{};
  auto k_r = k_ref.associatedNamespace(false, &k_s).asUTF8();
  DOODLE_CHICK(k_s);
  return k_r;
}
string reference_file::get_namespace() {
  chick_mobject();
  MFnReference k_ref{p_m_object};
  MStatus k_s{};
  auto k_r = k_ref.associatedNamespace(false, &k_s).asUTF8();
  DOODLE_CHICK(k_s);
  return k_r;
}
bool reference_file::replace_sim_assets_file() {
  chick_mobject();
  MFnReference k_ref{p_m_object};
  MStatus k_s{};


  if (!k_ref.isLoaded(&k_s)) {
    DOODLE_CHICK(k_s);
    return false;
  }

  auto k_prj = get_prj();
  if (!k_prj)
    return false;

  chick_component<project::cloth_config>(k_prj);
  auto k_vfx_path = k_prj.get<project::cloth_config>().vfx_cloth_sim_path / path;
  if (!FSys::exists(k_vfx_path))
    return false;

  auto k_comm = fmt::format(R"(
file -loadReference "{}" "{}";
)",
                            d_str{k_ref.name()}.str(), "");

  k_s         = MGlobal::executeCommand(d_str{k_comm});
  DOODLE_CHICK(k_s);
  return false;
}
entt::handle reference_file::get_prj() const {
  auto k_prj_view = g_reg()->view<project>();
  for (auto k_e : k_prj_view) {
    auto k_h = make_handle(k_e);
    if (k_h.get<database>() == prj_ref) {
      return k_h;
    }
  }
  return entt::handle{};
}
}  // namespace doodle::maya_plug
