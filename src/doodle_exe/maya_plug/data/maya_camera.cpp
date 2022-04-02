//
// Created by TD on 2021/12/13.
//

#include "maya_camera.h"
#include <maya/MTime.h>
#include <maya/MSelectionList.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>
#include <maya/MItDag.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnCamera.h>

#include <doodle_lib/metadata/export_file_info.h>
#include <doodle_lib/metadata/shot.h>
#include <doodle_lib/metadata/episodes.h>

#include <maya_plug/maya_plug_fwd.h>
#include <maya_plug/data/maya_file_io.h>
#include <regex>
namespace doodle::maya_plug {

maya_camera::maya_camera() = default;

maya_camera::maya_camera(const MDagPath& in_path)
    : maya_camera() {
  p_path = in_path;
  chick();
}

void maya_camera::chick() const {
  MStatus k_s{};
  chick_true<doodle_error>(p_path.isValid(&k_s), DOODLE_SOURCE_LOC, "无效的dag 路径");
  DOODLE_CHICK(k_s);
  chick_true<doodle_error>(p_path.hasFn(MFn::Type::kCamera, &k_s),
                           DOODLE_SOURCE_LOC,
                           "dag 路径不兼容 MFn::Type::kCamera");
  DOODLE_CHICK(k_s);
}

bool maya_camera::export_file(const MTime& in_start, const MTime& in_end) {
  chick();

  MStatus k_s{};
  MSelectionList k_select{};
  k_s = k_select.add(p_path);
  DOODLE_CHICK(k_s);
  k_s = MGlobal::setActiveSelectionList(k_select);
  DOODLE_CHICK(k_s);
  /// \brief 开始创建路径并进行导出
  auto k_file_path = maya_file_io::work_path("fbx") / maya_file_io::get_current_path().stem();
  if (!FSys::exists(k_file_path))
    FSys::create_directories(k_file_path);
  k_file_path /= fmt::format("{}_camera_{}-{}.fbx",
                             maya_file_io::get_current_path().stem().generic_string(),
                             in_start.value(),
                             in_end.value());
  auto k_comm = fmt::format("FBXExportBakeComplexStart -v {};", in_start.value());
  k_s         = MGlobal::executeCommand(d_str{k_comm});
  DOODLE_CHICK(k_s);

  k_comm = fmt::format("FBXExportBakeComplexEnd -v {};", in_end.value());
  k_s    = MGlobal::executeCommand(d_str{k_comm});
  DOODLE_CHICK(k_s);

  k_comm = string{"FBXExportBakeComplexAnimation -v true;"};
  k_s    = MGlobal::executeCommand(d_str{k_comm});
  DOODLE_CHICK(k_s);

  k_comm = string{"FBXExportConstraints -v true;"};
  k_s    = MGlobal::executeCommand(d_str{k_comm});
  DOODLE_CHICK(k_s);

  k_comm = fmt::format(R"(FBXExport -f "{}" -s;)", k_file_path.generic_string());
  k_s    = MGlobal::executeCommand(d_str{k_comm});
  DOODLE_CHICK(k_s);

  auto l_h = make_handle();

  episodes::analysis_static(l_h, k_file_path);
  shot::analysis_static(l_h, k_file_path);

  l_h.emplace<export_file_info>(k_file_path,
                                in_start.value(),
                                in_end.value(),
                                FSys::path{},
                                export_file_info::export_type::camera);
  export_file_info::write_file(l_h);
  return true;
}
bool maya_camera::back_camera(const MTime& in_start, const MTime& in_end) {
  chick();
  MStatus k_s{};

  MFnDagNode k_tran_node{};
  k_s = k_tran_node.setObject(p_path.transform());
  DOODLE_CHICK(k_s);

  auto k_comm = fmt::format(R"(bakeResults
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
                            in_start.value(), in_end.value(), d_str{k_tran_node.fullPathName()}.str());
  DOODLE_CHICK(k_s);
  k_s = MGlobal::executeCommand(d_str{k_comm});
  DOODLE_CHICK(k_s);
  return true;
}
bool maya_camera::unlock_attr() {
  chick();
  DOODLE_LOG_INFO("开始解除相机属性锁定");
  MStatus k_s{};
  MFnDependencyNode k_node{};
  {
    auto k_obj = p_path.node(&k_s);
    DOODLE_CHICK(k_s);
    k_s = k_node.setObject(k_obj);
    DOODLE_CHICK(k_s);
  }
  const auto& k_size = k_node.attributeCount(&k_s);
  DOODLE_CHICK(k_s);
  for (int l_i = 0; l_i < k_size; ++l_i) {
    auto k_attr = k_node.attribute(l_i, &k_s);
    DOODLE_CHICK(k_s);
    auto k_plug = k_node.findPlug(k_attr, false, &k_s);
    DOODLE_LOG_INFO("开始解锁属性 {}", k_plug.info());
    if (k_plug.isLocked(&k_s)) {
      DOODLE_CHICK(k_s);
      k_s = k_plug.setLocked(false);
      DOODLE_CHICK(k_s);
    }
  }
  {
    auto k_obj = p_path.transform(&k_s);
    DOODLE_CHICK(k_s);
    k_s = k_node.setObject(k_obj);
    DOODLE_CHICK(k_s);
  }
  const auto& k_size2 = k_node.attributeCount(&k_s);
  DOODLE_CHICK(k_s);
  for (int l_i = 0; l_i < k_size2; ++l_i) {
    auto k_attr = k_node.attribute(l_i, &k_s);
    DOODLE_CHICK(k_s);
    auto k_plug = k_node.findPlug(k_attr, false, &k_s);
    DOODLE_LOG_INFO("开始解锁属性 {}", k_plug.info());
    if (k_plug.isLocked(&k_s)) {
      DOODLE_CHICK(k_s);
      k_s = k_plug.setLocked(false);
      DOODLE_CHICK(k_s);
    }
  }

  return false;
}
void maya_camera::conjecture() {
  DOODLE_LOG_INFO("开始测量相机优先级");
  const static std::vector reg_list{
      regex_priority_pair{std::regex{"(front|persp|side|top|camera)"}, -1000},
      regex_priority_pair{std::regex{R"(ep\d+_sc\d+)", std::regex::icase}, 30},
      regex_priority_pair{std::regex{R"(ep\d+)", std::regex::icase}, 10},
      regex_priority_pair{std::regex{R"(sc\d+)", std::regex::icase}, 10},
      regex_priority_pair{std::regex{R"(ep_\d+_sc_\d+)", std::regex::icase}, 10},
      regex_priority_pair{std::regex{R"(ep_\d+)", std::regex::icase}, 5},
      regex_priority_pair{std::regex{R"(sc_\d+)", std::regex::icase}, 5},
      regex_priority_pair{std::regex{R"(^[A-Z]+_)"}, 2},
      regex_priority_pair{std::regex{R"(_\d+_\d+)", std::regex::icase}, 2}};

  MStatus k_s;
  MItDag k_it{MItDag::kBreadthFirst, MFn::kCamera, &k_s};
  DOODLE_CHICK(k_s);

  std::vector<camera> k_list{};
  for (; !k_it.isDone(); k_it.next()) {
    MDagPath k_path{};
    k_s = k_it.getPath(k_path);
    DOODLE_CHICK(k_s);
    string k_path_str = d_str{k_path.fullPathName(&k_s)};
    DOODLE_CHICK(k_s);

    camera k_cam{k_path, 0};
    for (const auto& k_reg : reg_list) {
      if (std::regex_search(k_path_str, k_reg.reg)) {
        k_cam.priority += k_reg.priority;
      }
    }
    k_list.push_back(std::move(k_cam));
  }

  std::sort(k_list.begin(), k_list.end(), [](auto& in_l, auto& in_r) {
    return in_l > in_r;
  });

  for (const auto& k_c : k_list) {
    DOODLE_LOG_INFO("相机 {} 优先级是 {}", k_c.p_dag_path.fullPathName(), k_c.priority);
  }

  chick_true<doodle_error>(!k_list.empty(), DOODLE_SOURCE_LOC, "没有找到任何相机");

  auto k_cam_ptr = g_reg()->try_ctx<maya_camera>();
  if (k_cam_ptr) {
    k_cam_ptr->p_path = k_list.front().p_dag_path;
  } else {
    this->p_path = k_list.front().p_dag_path;
    g_reg()->set<maya_camera>(*this);
  }
}
void maya_camera::set_render_cam() const {
  MStatus k_s;
  MItDag k_it{MItDag::kBreadthFirst, MFn::kCamera, &k_s};
  DOODLE_CHICK(k_s);
  for (; !k_it.isDone(); k_it.next()) {
    MDagPath k_path{};
    k_s = k_it.getPath(k_path);
    DOODLE_CHICK(k_s);

    MFnDagNode k_cam_fn{k_path, &k_s};
    DOODLE_CHICK(k_s);

    auto k_plug = k_cam_fn.findPlug("renderable", true, &k_s);
    DOODLE_CHICK(k_s);
    k_s = k_plug.setBool(k_cam_fn.object() == p_path.node(&k_s));
    DOODLE_CHICK(k_s);
  }
}
void maya_camera::set_play_attr() {
  MStatus k_s;
  MFnCamera k_cam_fn{p_path, &k_s};
  DOODLE_CHICK(k_s);
  k_s = k_cam_fn.setFilmFit(MFnCamera::FilmFit::kFillFilmFit);
  DOODLE_CHICK(k_s);
  k_s = k_cam_fn.setDisplayFilmGate(false);
  DOODLE_CHICK(k_s);
  k_s = k_cam_fn.setDisplayGateMask(false);
  DOODLE_CHICK(k_s);
  auto k_displayResolution = k_cam_fn.findPlug("displayResolution", true, &k_s);
  DOODLE_CHICK(k_s);
  k_s = k_displayResolution.setBool(false);
  DOODLE_CHICK(k_s);
  set_render_cam();
}
std::double_t maya_camera::focalLength() const {
  MStatus k_s;
  MFnCamera k_cam_fn{p_path, &k_s};
  DOODLE_CHICK(k_s);
  auto k_r = k_cam_fn.focalLength(&k_s);
  DOODLE_CHICK(k_s);
  return k_r;
}
string maya_camera::get_transform_name() const {
  MStatus k_s{};
  auto k_obj = p_path.transform(&k_s);
  DOODLE_CHICK(k_s);
  MFnDagNode k_node{k_obj, &k_s};
  DOODLE_CHICK(k_s);
  auto k_str = k_node.name(&k_s);
  DOODLE_CHICK(k_s);
  return d_str{k_str};
}

bool maya_camera::camera::operator<(const maya_camera::camera& in_rhs) const {
  return priority < in_rhs.priority;
}
bool maya_camera::camera::operator>(const maya_camera::camera& in_rhs) const {
  return in_rhs < *this;
}
bool maya_camera::camera::operator<=(const maya_camera::camera& in_rhs) const {
  return !(in_rhs < *this);
}
bool maya_camera::camera::operator>=(const maya_camera::camera& in_rhs) const {
  return !(*this < in_rhs);
}
}  // namespace doodle::maya_plug
