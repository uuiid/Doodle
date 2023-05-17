//
// Created by TD on 2021/11/30.
//

#include "reference_file.h"

#include <doodle_core/lib_warp/std_fmt_optional.h>
#include <doodle_core/lib_warp/std_warp.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/export_file_info.h>
#include <doodle_core/metadata/redirection_path_info.h>
#include <doodle_core/metadata/shot.h>

#include <maya_plug/data/find_duplicate_poly.h>
#include <maya_plug/data/maya_call_guard.h>
#include <maya_plug/data/maya_file_io.h>
#include <maya_plug/data/qcloth_shape.h>
#include <maya_plug/fmt/fmt_dag_path.h>
#include <maya_plug/fmt/fmt_select_list.h>
#include <maya_plug/main/maya_plug_fwd.h>

#include "entt/entity/fwd.hpp"
#include "exception/exception.h"
#include <array>
#include <maya/MDagPath.h>
#include <maya/MFileIO.h>
#include <maya/MFileObject.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnReference.h>
#include <maya/MItDag.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MItSelectionList.h>
#include <maya/MIteratorType.h>
#include <maya/MNamespace.h>
#include <maya/MSceneMessage.h>
#include <maya/MTime.h>
#include <maya/MUuid.h>
#include <string_view>
#include <vector>

namespace doodle::maya_plug {

namespace reference_file_ns {

FSys::path generate_file_path_base::operator()(const reference_file &in_ref) const {
  return get_path() / get_name(in_ref ? in_ref.get_namespace() : ""s);
}

bool generate_file_path_base::operator==(const generate_file_path_base &in) const noexcept {
  return std::tie(extract_reference_name, extract_scene_name, use_add_range, add_external_string) ==
         std::tie(in.extract_reference_name, in.extract_scene_name, in.use_add_range, in.add_external_string);
}

bool generate_file_path_base::operator<(const generate_file_path_base &in) const noexcept {
  return std::tie(extract_reference_name, extract_scene_name, use_add_range, add_external_string) <
         std::tie(in.extract_reference_name, in.extract_scene_name, in.use_add_range, in.add_external_string);
}
std::string generate_file_path_base::get_extract_scene_name(const std::string &in_name) const {
  std::string l_out_name{};

  if (!extract_scene_name.empty() && !format_scene_name.empty()) {
    try {
      std::regex const l_regex{extract_scene_name};
      l_out_name = std::regex_replace(in_name, l_regex, format_scene_name);
    } catch (const std::regex_error &in) {
      DOODLE_LOG_ERROR("提取 {} 场景名称 {} 异常 {}", extract_scene_name, in_name, in.what());
    }
  } else {
    l_out_name = in_name;
  }
  DOODLE_LOG_INFO("正则 {} 提取完成场景名称 {}", extract_scene_name, l_out_name);
  return l_out_name;
}
std::string generate_file_path_base::get_extract_reference_name(const std::string &in_name) const {
  std::string l_out_name{};
  if (!extract_reference_name.empty()) {
    try {
      std::regex const l_regex{extract_reference_name};
      l_out_name = std::regex_replace(in_name, l_regex, format_reference_name);
    } catch (const std::regex_error &in) {
      DOODLE_LOG_ERROR("提取 {} 引用 {} 异常 {}", in_name, extract_reference_name, in.what());
    }
  } else {
    l_out_name = in_name;
  }
  DOODLE_LOG_INFO("正则 {} 提取完成引用名称 {}", extract_reference_name, l_out_name);
  return l_out_name;
}

generate_abc_file_path::generate_abc_file_path(const entt::registry &in) : generate_file_path_base() {
  auto &l_cong           = in.ctx().get<project_config::base_config>();

  extract_reference_name = l_cong.abc_export_extract_reference_name;
  format_reference_name  = l_cong.abc_export_format_reference_name;
  extract_scene_name     = l_cong.abc_export_extract_scene_name;
  format_scene_name      = l_cong.abc_export_format_scene_name;
  use_add_range          = l_cong.abc_export_add_frame_range;
}

FSys::path generate_abc_file_path::get_path() const {
  auto k_path = maya_file_io::work_path(FSys::path{"abc"} / maya_file_io::get_current_path().stem());
  if (!exists(k_path)) {
    create_directories(k_path);
  }
  return k_path;
}
FSys::path generate_abc_file_path::get_name(const std::string &in_ref_name) const {
  auto l_name = fmt::format(
      "{}_{}"s, get_extract_scene_name(maya_file_io::get_current_path().stem().generic_string()),
      get_extract_reference_name(in_ref_name)
  );
  if (add_external_string) l_name = fmt::format("{}_{}", l_name, *add_external_string);

  if (use_add_range)
    l_name = fmt::format(
        "{}_{}-{}", l_name, begin_end_time.first.as(MTime::uiUnit()), begin_end_time.second.as(MTime::uiUnit())
    );

  l_name += ".abc";

  return FSys::path{l_name};
}

generate_abc_file_path::~generate_abc_file_path() = default;

generate_fbx_file_path::generate_fbx_file_path(const entt::registry &in) : generate_file_path_base() {
  auto &l_cong           = in.ctx().get<project_config::base_config>();
  camera_suffix          = l_cong.maya_camera_suffix;
  extract_reference_name = l_cong.abc_export_extract_reference_name;
  format_reference_name  = l_cong.abc_export_format_reference_name;
  extract_scene_name     = l_cong.abc_export_extract_scene_name;
  format_scene_name      = l_cong.abc_export_format_scene_name;
  use_add_range          = l_cong.abc_export_add_frame_range;
}

FSys::path generate_fbx_file_path::get_path() const {
  auto k_path = maya_file_io::work_path(FSys::path{"fbx"} / maya_file_io::get_current_path().stem());
  if (!exists(k_path)) {
    create_directories(k_path);
  }
  return k_path;
}
FSys::path generate_fbx_file_path::get_name(const std::string &in_ref_name) const {
  auto l_name = fmt::format(
      "{}_{}"s, get_extract_scene_name(maya_file_io::get_current_path().stem().generic_string()),
      is_camera_attr ? camera_suffix : get_extract_reference_name(in_ref_name)
  );
  if (add_external_string) l_name = fmt::format("{}_{}", l_name, *add_external_string);

  if (use_add_range)
    l_name = fmt::format(
        "{}_{}-{}", l_name, begin_end_time.first.as(MTime::uiUnit()), begin_end_time.second.as(MTime::uiUnit())
    );

  FSys::path l_path{l_name};
  l_path = l_path.generic_string();
  l_path += ".fbx";
  return l_path;
}

void generate_fbx_file_path::is_camera(bool in_is_camera) { is_camera_attr = in_is_camera; }
generate_fbx_file_path::~generate_fbx_file_path() = default;

}  // namespace reference_file_ns

reference_file::reference_file() : path(), use_sim(false), collision_model(), p_m_object(), file_namespace(){};

reference_file::reference_file(const std::string &in_maya_namespace) : reference_file() {
  set_namespace(in_maya_namespace);
}
void reference_file::set_path(const MObject &in_ref_node) {
  MStatus k_s{};
  MFnReference k_ref{in_ref_node, &k_s};
  DOODLE_MAYA_CHICK(k_s);
  path = d_str{k_ref.fileName(false, true, true, &k_s)};
  DOODLE_MAYA_CHICK(k_s);
  file_namespace = d_str{k_ref.associatedNamespace(false, &k_s)};
  DOODLE_MAYA_CHICK(k_s);
}

MSelectionList reference_file::get_collision_model() const {
  MSelectionList l_list{};
  for (const auto &str : collision_model) {
    DOODLE_LOG_INFO("添加碰撞体: {}", str);
    l_list.add(str.c_str(), true);
  }
  return l_list;
}
void reference_file::find_ref_node(const std::string &in_ref_uuid) {
  MStatus k_s;
  MFnReference k_file;
  for (MItDependencyNodes refIter(MFn::kReference); !refIter.isDone(); refIter.next()) {
    k_s = k_file.setObject(refIter.thisNode());
    DOODLE_MAYA_CHICK(k_s);
    if (k_file.uuid().asString().asUTF8() == in_ref_uuid) {
      p_m_object = refIter.thisNode();
      set_path(p_m_object);
    }
  }
}

void reference_file::chick_mobject() const { DOODLE_CHICK(!file_namespace.empty(), doodle_error{"名称空间为空"}); }
void reference_file::set_collision_model(const MSelectionList &in_list) {
  collision_model.clear();
  collision_model_show_str.clear();
  chick_mobject();
  MStatus k_s{};
  MDagPath l_path{};
  MFnDagNode l_node{};
  for (MItSelectionList l_it{in_list, MFn::Type::kMesh, &k_s}; !l_it.isDone(&k_s); l_it.next()) {
    DOODLE_MAYA_CHICK(k_s);
    k_s = l_it.getDagPath(l_path);
    DOODLE_MAYA_CHICK(k_s);
    auto k_obj = l_path.transform(&k_s);
    DOODLE_MAYA_CHICK(k_s);
    k_s = l_node.setObject(k_obj);
    DOODLE_MAYA_CHICK(k_s);
    collision_model_show_str.emplace_back(d_str{l_node.name(&k_s)});
    DOODLE_MAYA_CHICK(k_s);

    collision_model.emplace_back(d_str{l_node.fullPathName(&k_s)});
    DOODLE_MAYA_CHICK(k_s);
  }
}

void reference_file::init_show_name() {
  collision_model_show_str.clear();
  MStatus k_s{};
  MDagPath l_path{};
  MFnDagNode l_node{};
  for (MItSelectionList l_it{get_collision_model(), MFn::Type::kMesh, &k_s}; !l_it.isDone(&k_s); l_it.next()) {
    DOODLE_MAYA_CHICK(k_s);
    k_s = l_it.getDagPath(l_path);
    DOODLE_MAYA_CHICK(k_s);
    auto k_obj = l_path.transform(&k_s);
    DOODLE_MAYA_CHICK(k_s);
    k_s = l_node.setObject(k_obj);
    DOODLE_MAYA_CHICK(k_s);
    collision_model_show_str.emplace_back(d_str{l_node.name(&k_s)});
    DOODLE_MAYA_CHICK(k_s);
  }
}
std::string reference_file::get_namespace() const {
  /// \brief 再没有名称空间时, 我们使用引用名称计算并映射到导出名称中去
  DOODLE_CHICK(!file_namespace.empty(), doodle_error{"名称空间为空"});
  return file_namespace;
}

bool reference_file::replace_sim_assets_file() {
  if (!use_sim) {
    DOODLE_LOG_WARN("跳过不解算的文件 {}", path);
    return false;
  }

  chick_mobject();

  DOODLE_CHICK(this->find_ref_node(), doodle_error{"缺失引用"});
  MFnReference k_ref{p_m_object};
  MStatus k_s{};

  /// \brief 检查各种必须属性
  if (!k_ref.isLoaded(&k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    DOODLE_LOG_WARN("引用没有加载, 跳过!");
    return false;
  }

  auto &k_cfg = g_reg()->ctx().get<project_config::base_config>();
  FSys::path k_m_str{get_path()};
  DOODLE_MAYA_CHICK(k_s);
  auto k_vfx_path = k_cfg.vfx_cloth_sim_path /
                    fmt::format("{}_cloth{}", k_m_str.stem().generic_string(), k_m_str.extension().generic_string());
  DOODLE_LOG_INFO("推测资产路径 {}", k_vfx_path);
  if (!FSys::exists(k_vfx_path)) return false;

  /// \brief 替换引用文件
  {
    path = k_vfx_path.generic_string();
    maya_call_guard l_guard{MSceneMessage::addCheckReferenceCallback(
        MSceneMessage::kBeforeLoadReferenceCheck,
        [](bool *retCode, const MObject &referenceNode, MFileObject &file, void *clientData) {
          auto *self = reinterpret_cast<reference_file *>(clientData);
          file.setRawFullName(d_str{self->path});
          *retCode = FSys::exists(self->path);
        },
        this
    )};

    std::string l_s = d_str{MFileIO::loadReferenceByNode(p_m_object, &k_s)};
    DOODLE_LOG_INFO("替换完成引用文件 {}", l_s);
  }
  return true;
}

bool reference_file::has_node(const MSelectionList &in_list) {
  chick_mobject();
  MStatus k_s{};
  MObject k_node{};
  for (MItSelectionList k_iter{in_list, MFn::Type::kDependencyNode, &k_s}; !k_iter.isDone(); k_iter.next()) {
    k_s = k_iter.getDependNode(k_node);
    DOODLE_MAYA_CHICK(k_s);

    if (has_node(k_node)) return true;
  }
  return false;
}

bool reference_file::has_node(const MObject &in_node) const {
  chick_mobject();
  MStatus k_s{};
  auto k_objs = MNamespace::getNamespaceObjects(d_str{file_namespace}, false, &k_s);
  for (int l_i = 0; l_i < k_objs.length(); ++l_i) {
    if (k_objs[l_i] == in_node) return true;
  }

  return false;
}
bool reference_file::set_namespace(const std::string &in_namespace) {
  DOODLE_CHICK(!in_namespace.empty(), doodle_error{"空名称空间"});
  file_namespace = in_namespace.substr(1);
  find_ref_node();
  return has_chick_group();
}
bool reference_file::find_ref_node() {
  chick_mobject();
  if (!p_m_object.isNull()) return true;

  MStatus k_s;
  MFnReference k_file;
  DOODLE_LOG_INFO("名称空间 {} 开始寻找的引用", file_namespace);
  for (MItDependencyNodes refIter(MFn::kReference); !refIter.isDone(); refIter.next()) {
    k_s = k_file.setObject(refIter.thisNode());
    DOODLE_MAYA_CHICK(k_s);
    const auto &&k_mata_str = k_file.associatedNamespace(false, &k_s);
    if (k_mata_str == file_namespace.c_str()) {
      p_m_object = refIter.thisNode();
    }
  }
  if (p_m_object.isNull()) {
    DOODLE_LOG_INFO("名称空间 {} 没有引用文件,使用名称空间作为引用", file_namespace);
    path = file_namespace;
    return false;
  }

  MFnReference k_ref{p_m_object, &k_s};
  DOODLE_MAYA_CHICK(k_s);
  path = d_str{k_ref.fileName(false, true, true, &k_s)};
  DOODLE_LOG_INFO("获得引用路径 {} 名称空间 {}", path, file_namespace);
  return true;
}
void reference_file::qlUpdateInitialPose() const {
  DOODLE_LOG_INFO("开始更新解算文件 {} 中的布料初始化姿势 {}", get_namespace());
  MStatus l_status{};
  auto l_v = find_duplicate_poly{}(MNamespace::getNamespaceObjects(d_str{this->get_namespace()}, false, &l_status));
  DOODLE_MAYA_CHICK(l_status);

  for (auto &&[l_obj1, l_obj2] : l_v) {
    MSelectionList l_list{};
    DOODLE_MAYA_CHICK(l_list.add(l_obj1));
    DOODLE_MAYA_CHICK(l_list.add(l_obj2));
    DOODLE_MAYA_CHICK(MGlobal::setActiveSelectionList(l_list));
    DOODLE_MAYA_CHICK(MGlobal::executeCommand(d_str{"qlUpdateInitialPose;"}));
  }
}

entt::handle reference_file::export_file_select(
    const reference_file::export_arg &in_arg, const MSelectionList &in_list
) {
  entt::handle out_{};
  FSys::path l_path{};

  export_file_info::export_type l_type{};

  if (!l_path.empty() && g_reg()->ctx().get<project_config::base_config>().use_write_metadata) {
    out_ = make_handle();
    FSys::path l_ref_file{this->path};
    if (l_ref_file.empty()) {
      l_ref_file = this->get_namespace();
    }
    auto l_eps{""s};
    if (episodes::analysis_static(out_, l_path)) l_eps = fmt::to_string(out_.get<episodes>());
    auto l_upload_prefix = FSys::path{magic_enum::enum_name(l_type).data()} / l_eps;
    shot::analysis_static(out_, l_path);
    out_.emplace<export_file_info>(
            l_path, boost::numeric_cast<std::int32_t>(in_arg.start_p.value()),
            boost::numeric_cast<std::int32_t>(in_arg.end_p.value()), l_ref_file, l_type
    )
        .upload_path_ = l_upload_prefix;
    export_file_info::write_file(out_);
  }
  return out_;
}

bool reference_file::replace_file(const entt::handle &in_handle) {
  DOODLE_CHICK(in_handle.all_of<redirection_path_info>(), doodle_error{"缺失替换引用信息"});
  DOODLE_CHICK(!p_m_object.isNull(), doodle_error{"没有引用文件, 无法替换"});
  search_file_info = in_handle;
  MStatus k_s{};
  {
    maya_call_guard l_guard{MSceneMessage::addCheckReferenceCallback(
        MSceneMessage::kBeforeLoadReferenceCheck,
        [](bool *retCode, const MObject &referenceNode, MFileObject &file, void *clientData) {
          auto *self  = reinterpret_cast<reference_file *>(clientData);
          auto l_path = self->search_file_info.get<redirection_path_info>().get_replace_path();
          if (l_path) {
            MStatus k_s{};
            DOODLE_LOG_INFO("开始替换文件 {} 到 {}", self->path, *l_path);
            k_s = file.setRawFullName(d_str{l_path->generic_string()});
            DOODLE_MAYA_CHICK(k_s);
            *retCode = FSys::exists(*l_path);
          } else {
            *retCode = false;
          }
        },
        this
    )};

    std::string l_s = d_str{MFileIO::loadReferenceByNode(p_m_object, &k_s)};
    DOODLE_MAYA_CHICK(k_s);
    DOODLE_LOG_INFO("替换完成引用文件 {}", l_s);
  }
  auto l_name   = search_file_info.get<redirection_path_info>().get_replace_path()->stem().generic_string();
  auto l_name_d = l_name;
  for (int l_i = 1; l_i < 1000 && MNamespace::namespaceExists(d_str{l_name_d}); ++l_i) {
    l_name_d = fmt::format("{}{}", l_name, l_i);
  }
  DOODLE_LOG_INFO("确认名称空间 {}", l_name_d);

  DOODLE_LOG_INFO("开始重命名名称空间 {} 到 {}", get_namespace(), l_name_d);
  k_s = MNamespace::renameNamespace(d_str{get_namespace()}, d_str{l_name_d});
  DOODLE_MAYA_CHICK(k_s);
  file_namespace = l_name_d;
  DOODLE_CHICK(find_ref_node(), doodle_error{"没有在新的名称空间中查询到引用节点"});
  if (!has_chick_group()) DOODLE_LOG_WARN("没有在引用文件中找到 导出 组");
  return false;
}
FSys::path reference_file::get_path() const {
  MStatus k_s{};
  MFnReference k_ref{p_m_object, &k_s};
  DOODLE_MAYA_CHICK(k_s);
  FSys::path l_path = d_str{k_ref.fileName(true, true, false, &k_s)}.str();
  DOODLE_MAYA_CHICK(k_s);
  DOODLE_LOG_INFO("获取引用路径 {}", l_path);
  return l_path;
}
FSys::path reference_file::get_abs_path() const {
  MStatus k_s{};
  MFnReference k_ref{p_m_object, &k_s};
  DOODLE_MAYA_CHICK(k_s);
  FSys::path l_path = d_str{k_ref.fileName(false, false, false, &k_s)}.str();
  DOODLE_MAYA_CHICK(k_s);
  return l_path;
}

MSelectionList reference_file::get_all_object() const {
  MStatus k_s{};
  MSelectionList l_select;
  auto l_r = MNamespace::getNamespaceObjects(d_str{file_namespace}, false, &k_s);
  DOODLE_MAYA_CHICK(k_s);
  for (std::uint32_t i = 0u; i < l_r.length(); ++i) {
    k_s = l_select.add(l_r[i], true);
    DOODLE_MAYA_CHICK(k_s);
  }
  return l_select;
}
std::optional<MDagPath> reference_file::export_group_attr() const {
  chick_mobject();
  MStatus k_s{};

  DOODLE_MAYA_CHICK(k_s);
  MSelectionList k_select{};
  auto &k_cfg = g_reg()->ctx().get<project_config::base_config>();
  MDagPath l_path;
  try {
    k_s = k_select.add(d_str{fmt::format("{}:{}", get_namespace(), k_cfg.export_group)}, true);
    DOODLE_MAYA_CHICK(k_s);
    k_s = k_select.getDagPath(0, l_path);
    DOODLE_MAYA_CHICK(k_s);
  } catch (const std::runtime_error &err) {
    DOODLE_LOG_INFO("引用文件 {} 没有配置中指定的 {} 导出组", get_namespace(), k_cfg.export_group);
  }
  return l_path.isValid() ? std::make_optional(l_path) : std::optional<MDagPath>{};
}

void reference_file::bake_results(const MTime &in_start, const MTime &in_end) const {
  if (!has_chick_group()) {
    DOODLE_LOG_INFO("{} 没有ue4组", path);
    return;
  }

  MStatus k_s{};
  auto &k_cfg = g_reg()->ctx().get<project_config::base_config>();
  /**
   *
   * @brief
   * bakeResults(simulation=True,
   *  time=(doodle_work_space.raneg.start,
   *        doodle_work_space.raneg.end),
   *  hierarchy="below",
   *  sampleBy=1,
   *  disableImplicitControl=True,
   *  preserveOutsideKeys=False,
   *  sparseAnimCurveBake=False)
   *
   *  preserveOutsideKeys 这个选项会导致眼睛出现问题
   */
  static std::string maya_bakeResults_str{R"(
bakeResults -simulation true -t "{}:{}" -hierarchy below -sampleBy 1 -oversamplingRate 1 -disableImplicitControl true -preserveOutsideKeys {} -sparseAnimCurveBake false -removeBakedAttributeFromLayer false -removeBakedAnimFromLayer false -bakeOnOverrideLayer false -minimizeRotation true -controlPoints false -shape true "{}:*{}";)"};
  auto l_comm = fmt::format(
      maya_bakeResults_str, in_start.value(), in_end.value(), "false"s, get_namespace(), k_cfg.export_group
  );
  DOODLE_LOG_INFO("开始使用命令 {} 主动烘培动画帧", l_comm);
  try {
    k_s = MGlobal::executeCommand(d_str{l_comm});
    DOODLE_MAYA_CHICK(k_s);
  } catch (const std::runtime_error &in) {
    DOODLE_LOG_INFO("开始主动烘培动画帧失败, 开始使用备用参数重试 {}", boost::diagnostic_information(in));
    try {
      l_comm = fmt::format(
          maya_bakeResults_str, in_start.value(), in_end.value(), "true"s, get_namespace(), k_cfg.export_group
      );
      DOODLE_LOG_INFO("开始使用命令 {} 主动烘培动画帧", l_comm);
      k_s = MGlobal::executeCommand(d_str{l_comm});
      DOODLE_MAYA_CHICK(k_s);
    } catch (const std::runtime_error &in2) {
      DOODLE_LOG_INFO("开始主动烘培动画帧失败, 开始使用默认参数重试  error {} ", boost::diagnostic_information(in2));

      try {
        l_comm = fmt::format(
            R"(bakeResults  -simulation true -t "{}:{}" -hierarchy below "{}:*{}";)", in_start.value(), in_end.value(),
            get_namespace(), k_cfg.export_group
        );
        DOODLE_LOG_INFO("开始使用命令 {} 主动烘培动画帧", l_comm);
        k_s = MGlobal::executeCommand(d_str{l_comm});
        DOODLE_MAYA_CHICK(k_s);
      } catch (const std::runtime_error &in3) {
        DOODLE_LOG_INFO("烘培失败, 直接导出 {}", boost::diagnostic_information(in3));
      }
    }

    DOODLE_LOG_INFO("完成烘培, 不检查结果, 直接进行输出");
  }
}

std::optional<MDagPath> reference_file::get_field_dag() const {
  MSelectionList l_select{};
  auto l_status = l_select.add(d_str{field_attr}, false);
  if (l_status == MStatus::kInvalidParameter) {
    return {};
  } else {
    MDagPath l_obj{};
    l_status = l_select.getDagPath(0, l_obj);
    DOODLE_MAYA_CHICK(l_status);
    return l_obj;
  }
}
void reference_file::add_field_dag(const MSelectionList &in_list) {
  MStatus l_status{};
  MItSelectionList l_it{in_list, MFn::kField, &l_status};
  DOODLE_MAYA_CHICK(l_status);
  for (; !l_it.isDone(); l_it.next()) {
    MDagPath l_path{};

    l_status = l_it.getDagPath(l_path);
    DOODLE_MAYA_CHICK(l_status);
    field_attr = fmt::to_string(l_path);

    return;
  }
}
const std::string &reference_file::get_field_string() const { return field_attr; }
const std::string &reference_file::get_key_path() const { return path; }
bool reference_file::has_chick_group() const {
  auto &k_cfg = g_reg()->ctx().get<project_config::base_config>();
  try {
    chick_mobject();
    MStatus k_s{};

    MSelectionList k_select{};
    k_s = k_select.add(d_str{fmt::format("{}:{}", get_namespace(), k_cfg.export_group)}, true);
    maya_chick(k_s);
    return true;
  } catch (const maya_error &err) {
    DOODLE_LOG_INFO("引用文件 {} 没有配置中指定的 {} 导出组 {}", file_namespace, k_cfg.export_group, err.what());
    return false;
  }
}

std::vector<entt::handle> reference_file_factory::create_ref() const {
  std::vector<entt::handle> l_ret{};
  g_reg()->clear<reference_file>();
  g_reg()->clear<qcloth_shape>();
  MStatus k_s;
  auto k_names = MNamespace::getNamespaces(MNamespace::rootNamespace(), false, &k_s);
  maya_chick(k_s);

  constexpr static std::array<std::string_view, 2> g_not_find_ui{":UI", ":shared"};
  for (int l_i = 0; l_i < k_names.length(); ++l_i) {
    auto &&k_name = k_names[l_i];
    if (std::find(g_not_find_ui.begin(), g_not_find_ui.end(), k_name.asUTF8()) != g_not_find_ui.end()) {
      continue;
    }
    reference_file k_ref{k_name};
    if (k_ref) {
      DOODLE_LOG_INFO("获得引用文件 {}", k_ref.get_key_path());
      auto l_h = make_handle();
      l_h.emplace<reference_file>(k_ref);
      l_ret.emplace_back(l_h);
    } else {
      DOODLE_LOG_INFO("引用文件 {} 未加载", k_ref.get_key_path());
    }
  }

  return l_ret;
}
void reference_file_factory::save_to_files() const {}

}  // namespace doodle::maya_plug
