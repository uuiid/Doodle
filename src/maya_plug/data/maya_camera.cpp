//
// Created by TD on 2021/12/13.
//

#include "maya_camera.h"

#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/export_file_info.h>
#include <doodle_core/metadata/shot.h>

#include <maya_plug/data/maya_file_io.h>
#include <maya_plug/data/reference_file.h>
#include <maya_plug/main/maya_plug_fwd.h>

#include <maya/MDGModifier.h>
#include <maya/MFnCamera.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MItDag.h>
#include <maya/MPlug.h>
#include <maya/MSelectionList.h>
#include <maya/MTime.h>
#include <regex>
namespace doodle::maya_plug {

maya_camera::maya_camera() = default;

maya_camera::maya_camera(const MDagPath& in_path) : maya_camera() {
  p_path = in_path;
  chick();
}

void maya_camera::chick() const {
  MStatus k_s{};
  DOODLE_CHICK(p_path.isValid(&k_s), doodle_error{"无效的dag 路径"s});
  DOODLE_MAYA_CHICK(k_s);
  DOODLE_CHICK(p_path.hasFn(MFn::Type::kCamera, &k_s), doodle_error{"dag 路径不兼容 MFn::Type::kCamera"s});
  DOODLE_MAYA_CHICK(k_s);
}

bool maya_camera::export_file(
    const MTime& in_start, const MTime& in_end, const reference_file_ns::generate_fbx_file_path& in_name
) {
  chick();

  MStatus k_s{};
  MSelectionList k_select{};
  k_s = k_select.add(p_path);
  DOODLE_MAYA_CHICK(k_s);
  k_s = MGlobal::setActiveSelectionList(k_select);
  DOODLE_MAYA_CHICK(k_s);
  /// \brief 开始创建路径并进行导出
  auto l_name = in_name;
  l_name.is_camera(true);
  auto k_file_path = l_name({});
  auto k_comm      = fmt::format("FBXExportBakeComplexStart -v {};", in_start.value());
  k_s              = MGlobal::executeCommand(d_str{k_comm});
  DOODLE_MAYA_CHICK(k_s);

  k_comm = fmt::format("FBXExportBakeComplexEnd -v {};", in_end.value());
  k_s    = MGlobal::executeCommand(d_str{k_comm});
  DOODLE_MAYA_CHICK(k_s);

  k_comm = std::string{"FBXExportBakeComplexAnimation -v true;"};
  k_s    = MGlobal::executeCommand(d_str{k_comm});
  DOODLE_MAYA_CHICK(k_s);

  k_comm = std::string{"FBXExportConstraints -v true;"};
  k_s    = MGlobal::executeCommand(d_str{k_comm});
  DOODLE_MAYA_CHICK(k_s);

  k_comm = fmt::format(R"(FBXExport -f "{}" -s;)", k_file_path.generic_string());
  k_s    = MGlobal::executeCommand(d_str{k_comm});
  DOODLE_MAYA_CHICK(k_s);

  if (g_reg()->ctx().get<project_config::base_config>().use_write_metadata) {
    auto l_h = make_handle();
    episodes::analysis_static(l_h, k_file_path);
    shot::analysis_static(l_h, k_file_path);

    l_h.emplace<export_file_info>(
        k_file_path, in_start.value(), in_end.value(), FSys::path{}, export_file_info::export_type::camera
    );
    export_file_info::write_file(l_h);
  }
  return true;
}
bool maya_camera::back_camera(const MTime& in_start, const MTime& in_end) {
  chick();
  MStatus k_s{};

  MFnDagNode k_tran_node{};
  k_s = k_tran_node.setObject(p_path.transform());
  DOODLE_MAYA_CHICK(k_s);

  auto k_comm = fmt::format(
      R"(bakeResults
-simulation true
-t "{}:{}"
-hierarchy below
-sampleBy 1
-oversamplingRate 1
-disableImplicitControl true
-preserveOutsideKeys true
-sparseAnimCurveBake false
-removeBakedAttributeFromLayer false
-removeBakedAnimFromLayer false
-bakeOnOverrideLayer false
-minimizeRotation true
-controlPoints false
-shape true
{{"{}"}};
)",
      in_start.value(), in_end.value(), d_str{k_tran_node.fullPathName()}.str()
  );
  DOODLE_MAYA_CHICK(k_s);
  k_s = MGlobal::executeCommand(d_str{k_comm});
  DOODLE_MAYA_CHICK(k_s);
  return true;
}
bool maya_camera::unlock_attr() {
  chick();
  DOODLE_LOG_INFO("开始解除相机属性锁定");
  MStatus k_s{};
  MFnDependencyNode k_node{};
  {
    auto k_obj = p_path.node(&k_s);
    DOODLE_MAYA_CHICK(k_s);
    k_s = k_node.setObject(k_obj);
    DOODLE_MAYA_CHICK(k_s);
  }
  const auto& k_size = k_node.attributeCount(&k_s);
  DOODLE_MAYA_CHICK(k_s);
  for (int l_i = 0; l_i < k_size; ++l_i) {
    auto k_attr = k_node.attribute(l_i, &k_s);
    DOODLE_MAYA_CHICK(k_s);
    auto k_plug = k_node.findPlug(k_attr, false, &k_s);
    //    DOODLE_LOG_INFO("开始解锁属性 {}", k_plug.info());
    if (k_plug.isLocked(&k_s)) {
      DOODLE_MAYA_CHICK(k_s);
      k_s = k_plug.setLocked(false);
      DOODLE_MAYA_CHICK(k_s);
    }
  }
  {
    auto k_obj = p_path.transform(&k_s);
    DOODLE_MAYA_CHICK(k_s);
    k_s = k_node.setObject(k_obj);
    DOODLE_MAYA_CHICK(k_s);
  }
  const auto& k_size2 = k_node.attributeCount(&k_s);
  DOODLE_MAYA_CHICK(k_s);
  for (int l_i = 0; l_i < k_size2; ++l_i) {
    auto k_attr = k_node.attribute(l_i, &k_s);
    DOODLE_MAYA_CHICK(k_s);
    auto k_plug = k_node.findPlug(k_attr, false, &k_s);
    //    DOODLE_LOG_INFO("开始解锁属性 {}", k_plug.info());
    if (k_plug.isLocked(&k_s)) {
      DOODLE_MAYA_CHICK(k_s);
      k_s = k_plug.setLocked(false);
      DOODLE_MAYA_CHICK(k_s);
    }
  }

  return false;
}
void maya_camera::conjecture() {
  DOODLE_LOG_INFO("开始测量相机优先级");

  auto l_reg_list =
      g_reg()->ctx().get<project_config::base_config>().maya_camera_select |
      ranges::views::transform([](const project_config::camera_judge& in_camera_judge) -> regex_priority_pair {
        return regex_priority_pair{std::regex{in_camera_judge.first, std::regex::icase}, in_camera_judge.second};
      }) |
      ranges::to_vector;

  MStatus k_s;
  MItDag k_it{MItDag::kBreadthFirst, MFn::kCamera, &k_s};
  DOODLE_MAYA_CHICK(k_s);

  std::vector<camera> k_list{};
  for (; !k_it.isDone(); k_it.next()) {
    MDagPath k_path{};
    k_s = k_it.getPath(k_path);
    DOODLE_MAYA_CHICK(k_s);
    std::string k_path_str = d_str{k_path.fullPathName(&k_s)};
    DOODLE_MAYA_CHICK(k_s);

    camera k_cam{k_path, 0};
    for (const auto& k_reg : l_reg_list) {
      if (std::regex_search(k_path_str, k_reg.reg)) {
        k_cam.priority += k_reg.priority;
      }
    }
    k_list.push_back(std::move(k_cam));
  }

  std::sort(k_list.begin(), k_list.end(), [](auto& in_l, auto& in_r) { return in_l > in_r; });

  for (const auto& k_c : k_list) {
    DOODLE_LOG_INFO("相机 {} 优先级是 {}", k_c.p_dag_path.fullPathName(), k_c.priority);
  }

  DOODLE_CHICK(!k_list.empty(), doodle_error{"没有找到任何相机"s});

  if (g_reg()->ctx().contains<maya_camera>()) {
    g_reg()->ctx().get<maya_camera>().p_path = k_list.front().p_dag_path;
  } else {
    this->p_path = k_list.front().p_dag_path;
    g_reg()->ctx().emplace<maya_camera>(*this);
  }
}
void maya_camera::set_render_cam() const {
  MStatus k_s;
  MItDag k_it{MItDag::kBreadthFirst, MFn::kCamera, &k_s};
  DOODLE_MAYA_CHICK(k_s);
  for (; !k_it.isDone(); k_it.next()) {
    MDagPath k_path{};
    k_s = k_it.getPath(k_path);
    DOODLE_MAYA_CHICK(k_s);

    MFnDagNode k_cam_fn{k_path, &k_s};
    DOODLE_MAYA_CHICK(k_s);

    auto k_plug = k_cam_fn.findPlug("renderable", true, &k_s);
    DOODLE_MAYA_CHICK(k_s);
    k_s = k_plug.setBool(k_cam_fn.object() == p_path.node(&k_s));
    DOODLE_MAYA_CHICK(k_s);
  }
}
void maya_camera::set_play_attr() {
  MStatus k_s;
  MFnCamera k_cam_fn{p_path, &k_s};
  DOODLE_MAYA_CHICK(k_s);
  k_s = k_cam_fn.setFilmFit(MFnCamera::FilmFit::kFillFilmFit);
  DOODLE_MAYA_CHICK(k_s);
  k_s = k_cam_fn.setDisplayFilmGate(false);
  DOODLE_MAYA_CHICK(k_s);
  k_s = k_cam_fn.setDisplayGateMask(false);
  DOODLE_MAYA_CHICK(k_s);
  k_s = k_cam_fn.setNearClippingPlane(1);
  DOODLE_MAYA_CHICK(k_s);

  auto k_displayResolution = k_cam_fn.findPlug("displayResolution", true, &k_s);
  DOODLE_MAYA_CHICK(k_s);
  k_s = k_displayResolution.setBool(false);
  DOODLE_MAYA_CHICK(k_s);
  set_render_cam();
}
std::double_t maya_camera::focalLength() const {
  MStatus k_s;
  MFnCamera k_cam_fn{p_path, &k_s};
  DOODLE_MAYA_CHICK(k_s);
  auto k_r = k_cam_fn.focalLength(&k_s);
  DOODLE_MAYA_CHICK(k_s);
  return k_r;
}
std::string maya_camera::get_transform_name() const {
  MStatus k_s{};
  auto k_obj = p_path.transform(&k_s);
  DOODLE_MAYA_CHICK(k_s);
  MFnDagNode k_node{k_obj, &k_s};
  DOODLE_MAYA_CHICK(k_s);
  auto k_str = k_node.name(&k_s);
  DOODLE_MAYA_CHICK(k_s);
  return d_str{k_str};
}
std::string maya_camera::get_full_name() const {
  MStatus k_s{};
  auto k_obj = p_path.transform(&k_s);
  maya_chick(k_s);

  return get_node_full_name(k_obj);
}

bool maya_camera::fix_group_camera(const MTime& in_start, const MTime& in_end) {
  MFnDagNode l_node{};
  MStatus l_s{};
  l_s = l_node.setObject(p_path.transform());
  DOODLE_MAYA_CHICK(l_s);
  if (p_path.length() > 1) {
    DOODLE_LOG_INFO("测量到相机 {} 有组父物体, 开始转换相机", get_transform_name());
    /// \brief 开始调整相机并创建新相机
    MFnCamera l_camera{};
    l_camera.create(&l_s);
    DOODLE_MAYA_CHICK(l_s);
    /// 创建约束
    auto l_cam_name = get_node_name(get_transform(l_camera.object()));
    auto l_comm     = fmt::format("parentConstraint -weight 1 {} {};", l_node.fullPathName(), l_cam_name);

    DOODLE_LOG_INFO("运行 {}", l_comm);
    MStringArray l_constraints{};
    l_s = MGlobal::executeCommand(d_str{l_comm}, l_constraints);
    DOODLE_MAYA_CHICK(l_s);
    auto l_old_path{p_path};

    l_s = l_camera.getPath(p_path);
    DOODLE_MAYA_CHICK(l_s);

    back_camera(in_start, in_end);
    /// \brief 删除约束
    {
      DOODLE_LOG_INFO("删除约束 {}", l_constraints);
      MSelectionList l_select{};
      for (int l_i = 0; l_i < l_constraints.length(); ++l_i) {
        l_s = l_select.add(l_constraints[l_i]);
        DOODLE_MAYA_CHICK(l_s);
      }
      MObject l_con{};
      for (int l_i = 0; l_i < l_select.length(); ++l_i) {
        l_s = l_select.getDependNode(l_i, l_con);
        DOODLE_MAYA_CHICK(l_s);
        l_s = MGlobal::deleteNode(l_con);
        DOODLE_MAYA_CHICK(l_s);
      }
    }

    MDGModifier l_dag_modifier{};
#define DOODLE_CONN_CAM(attr_name)                                         \
  {                                                                        \
    auto l_so_plug  = get_plug(l_old_path.node(), #attr_name##s).source(); \
    auto l_t        = get_plug(l_camera.object(), #attr_name##s);          \
    auto l_dis_dest = get_plug(l_camera.object(), #attr_name##s).source(); \
    l_dag_modifier.disconnect(l_dis_dest, l_t);                            \
    l_dag_modifier.connect(l_so_plug, l_t);                                \
  }
    DOODLE_CONN_CAM(horizontalFilmAperture)
    DOODLE_CONN_CAM(verticalFilmAperture)
    DOODLE_CONN_CAM(centerOfInterest)
    DOODLE_CONN_CAM(fStop)
    DOODLE_CONN_CAM(focalLength)
    DOODLE_CONN_CAM(focusDistance)
    DOODLE_CONN_CAM(lensSqueezeRatio)
    DOODLE_CONN_CAM(shutterAngle)
#undef DOODLE_CONN_CAM

    l_s = l_dag_modifier.doIt();
    DOODLE_MAYA_CHICK(l_s);

    return true;
  }

  return false;
}
bool maya_camera::camera_parent_is_word() {
  MFnDagNode l_node{};
  MStatus l_s{};
  l_s = l_node.setObject(p_path.transform());
  DOODLE_MAYA_CHICK(l_s);
  MDagPath l_path{};
  l_s = l_node.getPath(l_path);
  DOODLE_MAYA_CHICK(l_s);

  DOODLE_LOG_INFO("检查相机级数为 {}", l_path.length());
  return l_path.length() > 1;
}

bool maya_camera::camera::operator<(const maya_camera::camera& in_rhs) const { return priority < in_rhs.priority; }
bool maya_camera::camera::operator>(const maya_camera::camera& in_rhs) const { return in_rhs < *this; }
bool maya_camera::camera::operator<=(const maya_camera::camera& in_rhs) const { return !(in_rhs < *this); }
bool maya_camera::camera::operator>=(const maya_camera::camera& in_rhs) const { return !(*this < in_rhs); }
}  // namespace doodle::maya_plug
