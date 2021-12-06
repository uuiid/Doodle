//
// Created by TD on 2021/12/6.
//

#include "qcloth_shape.h"

#include <doodle_lib/metadata/project.h>
#include <maya/MFileIO.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>
#include <maya_plug/command/reference_file.h>
#include <maya_plug/data/maya_file_io.h>
#include <maya_plug/maya_plug_fwd.h>
namespace doodle::maya_plug {
qcloth_shape::qcloth_shape() = default;

qcloth_shape::qcloth_shape(const entt::handle& in_ref_file, const MObject& in_object)
    : qcloth_shape() {
  p_ref_file = in_ref_file;
  obj        = in_object;
  chick_component<reference_file>(p_ref_file);
}
bool qcloth_shape::set_cache_folder() const {
  MStatus k_s{};
  MFnDependencyNode k_node{obj, &k_s};
  string k_name{k_node.name().asUTF8()};
  auto& k_cfg = p_ref_file.get<reference_file>().get_prj().get<project::cloth_config>();
  boost::replace_all_copy(k_name, k_cfg.cloth_proxy, k_cfg.cloth_shape);

  /// 选择解算节点
  MSelectionList l_selection_list{};
  k_s = l_selection_list.add(d_str{k_name}, true);
  DOODLE_CHICK(k_s);
  if (l_selection_list.length(&k_s) > 1) {
    throw doodle_error{"出现重名物体"};
  }
  if (l_selection_list.isEmpty(&k_s)) {
    throw doodle_error{"没有找到解算布料节点"};
  }

  /// \brief 获得解算节点fn
  MObject k_shape{};
  k_s = l_selection_list.getDependNode(0, k_shape);
  DOODLE_CHICK(k_s);
  k_s = k_node.setObject(k_shape);
  DOODLE_CHICK(k_s);
  string k_node_name = k_node.name(&k_s).asUTF8();
  {
    auto k_cache = k_node.findPlug(d_str{"cacheFolder"}, true, &k_s);
    DOODLE_CHICK(k_s);
    auto k_file_name   = maya_file_io::get_current_path();
    string k_namespace = p_ref_file.get<reference_file>().get_namespace();
    DOODLE_CHICK(k_s);
    k_cache.setString(d_str{fmt::format("cache/{}/{}/{}", k_file_name.stem().generic_string(), k_namespace, k_node_name)});
  }
  {
    auto k_cache = k_node.findPlug(d_str{"cacheName"}, true, &k_s);
    DOODLE_CHICK(k_s);
    k_cache.setString(d_str{k_node_name});
  }
  return false;
}
bool qcloth_shape::create_cache() const {
  return false;
}

}  // namespace doodle::maya_plug
