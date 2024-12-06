//
// Created by TD on 2021/12/13.
//

#include "maya_camera.h"

#include <doodle_core/metadata/episodes.h>
#include <doodle_core/platform/win/register_file_type.h>

#include <maya_plug/data/maya_file_io.h>
#include <maya_plug/data/reference_file.h>
#include <maya_plug/fmt/fmt_dag_path.h>
#include <maya_plug/main/maya_plug_fwd.h>

#include "exception/exception.h"
#include "maya_conv_str.h"
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
  k_s = MGlobal::executeCommand(conv::to_ms(k_comm));
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
maya_camera maya_camera::conjecture() {
  {
    default_logger_raw()->warn("先旧版本相机测试");
    static auto l_list = std::vector{
        {std::make_pair(R"(front|persp|side|top|camera)"s, -1000), std::make_pair(R"(ep\d+_sc\d+)"s, 30),
         std::make_pair(R"(ep\d+)"s, 10), std::make_pair(R"(sc\d+)"s, 10), std::make_pair(R"(ep_\d+_sc_\d+)"s, 10),
         std::make_pair(R"(ep_\d+)"s, 5), std::make_pair(R"(sc_\d+)"s, 5), std::make_pair(R"(^[A-Z]+_)"s, 2),
         std::make_pair(R"(_\d+_\d+)"s, 2)}
    };
    auto l_reg_list =
        l_list |
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
    if (!k_list.empty()) {
      return maya_camera{k_list[0].p_dag_path};
    }
  }
  default_logger_raw()->warn("新版本相机测试");
  MStatus k_s;
  MItDag k_it{MItDag::kBreadthFirst, MFn::kCamera, &k_s};
  DOODLE_MAYA_CHICK(k_s);
  MDagPath l_cam_dag_path{};

  static std::regex l_re{R"(^[A-Z]{2}_EP\d+_SC\d+[A-Z]?)"};

  auto l_s_name = maya_file_io::get_current_path().stem().generic_string();
  for (; !k_it.isDone(); k_it.next()) {
    MDagPath k_path{};
    k_s = k_it.getPath(k_path);
    DOODLE_MAYA_CHICK(k_s);
    std::string k_path_str = d_str{k_path.fullPathName(&k_s)};
    DOODLE_MAYA_CHICK(k_s);

    auto l_sub = k_path_str.substr(1, k_path_str.find('|', 1) - 1);
    default_logger_raw()->warn("获取场景名称 {}, 并开始测试相机 {}", l_s_name, l_sub);
    if (std::regex_search(l_sub, l_re) && l_sub == l_s_name) {
      l_cam_dag_path = k_path;
      break;
    }
  }

  if (!l_cam_dag_path.isValid()) {
    default_logger_raw()->error("没有找到任何相机");
    throw_error(maya_enum::maya_error_t::camera_name_error);
  }
  default_logger_raw()->warn("找到相机 {}", l_cam_dag_path);
  return maya_camera{l_cam_dag_path};
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
  maya_chick(k_cam_fn.setNearClippingPlane(10));

  MGlobal::executeCommand(R"(setAttr "hardwareRenderingGlobals.multiSampleEnable" 1;)");

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
