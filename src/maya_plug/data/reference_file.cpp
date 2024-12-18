//
// Created by TD on 2021/11/30.
//

#include "reference_file.h"

#include "doodle_core/exception/exception.h"
#include "doodle_core/logger/logger.h"
#include <doodle_core/lib_warp/std_fmt_optional.h>
#include <doodle_core/lib_warp/std_warp.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/shot.h>

#include <boost/lambda2.hpp>
#include <boost/locale.hpp>

#include <maya_plug/data/m_namespace.h>
#include <maya_plug/data/maya_call_guard.h>
#include <maya_plug/data/maya_file_io.h>
#include <maya_plug/data/qcloth_shape.h>
#include <maya_plug/fmt/fmt_dag_path.h>
#include <maya_plug/fmt/fmt_select_list.h>
#include <maya_plug/node/files_info.h>

#include "entt/entity/fwd.hpp"
#include "exception/exception.h"
#include "maya_conv_str.h"
#include "maya_tool.h"
#include <array>
#include <filesystem>
#include <maya/MApiNamespace.h>
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
  return get_path() / get_name(in_ref.export_group_attr().has_value() ? in_ref.get_namespace() : ""s);
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

generate_fbx_file_path::generate_fbx_file_path() : generate_file_path_base() { camera_suffix = "camera"s; }

FSys::path generate_fbx_file_path::get_path() const {
  auto k_path = maya_file_io::work_path(FSys::path{"fbx"} / maya_file_io::get_current_path().stem());
  if (!exists(k_path)) {
    create_directories(k_path);
  }
  return k_path;
}
FSys::path generate_fbx_file_path::get_name(const std::string &in_ref_name) const {
  auto l_name = fmt::format(
      "{}_{}", get_extract_scene_name(maya_file_io::get_current_path().stem().generic_string()),
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

generate_abc_file_path::generate_abc_file_path() : generate_fbx_file_path() {}

FSys::path generate_abc_file_path::get_path() const {
  auto k_path = maya_file_io::work_path(FSys::path{"abc"} / maya_file_io::get_current_path().stem());
  if (!exists(k_path)) {
    create_directories(k_path);
  }
  return k_path;
}
FSys::path generate_abc_file_path::get_name(const std::string &in_ref_name) const {
  auto l_path = generate_fbx_file_path::get_name(in_ref_name);

  l_path.replace_extension(export_fbx ? ".fbx" : ".abc");

  return l_path;
}

generate_abc_file_path::~generate_abc_file_path() = default;

void generate_fbx_file_path::is_camera(bool in_is_camera) { is_camera_attr = in_is_camera; }
generate_fbx_file_path::~generate_fbx_file_path() = default;

}  // namespace reference_file_ns

reference_file::reference_file() = default;
reference_file::reference_file(const MObject &in_ref_node) : file_info_node_(in_ref_node) {}

std::string reference_file::get_file_namespace() const {
  return file_info_node_.isNull() ? std::string{}
                                  : conv::to_s(get_plug(file_info_node_, "reference_file_namespace").asString());
}

void reference_file::set_use_sim(bool in_use_sim) { set_attribute(file_info_node_, "is_solve", in_use_sim); }

bool reference_file::get_use_sim() const { return get_attribute<bool>(file_info_node_, "is_solve"); }

MObject reference_file::get_ref_node() const {
  MStatus l_status{};

  MFnDependencyNode l_file_info{};
  maya_chick(l_file_info.setObject(file_info_node_));
  auto l_plug = l_file_info.findPlug("reference_file", false, &l_status);
  maya_chick(l_status);
  if (l_plug.isConnected()) {
    auto l_node_plug = l_plug.source(&l_status);
    maya_chick(l_status);
    auto l_node = l_node_plug.node(&l_status);
    maya_chick(l_status);

    return l_node;
  } else {
    default_logger_raw()->log(log_loc(), spdlog::level::err, "引用文件 {} 没有连接文件", get_namespace());
  }
  return {};
}

bool reference_file::is_loaded() const {
  MStatus l_status{};
  auto l_node = get_ref_node();
  if (l_node.isNull()) {
    default_logger_raw()->log(log_loc(), spdlog::level::err, "引用文件 {} 没有连接文件", get_namespace());
    return false;
  }
  MFnReference l_ref{};
  maya_chick(l_ref.setObject(l_node));
  auto l_ret = l_ref.isLoaded(&l_status);
  maya_chick(l_status);
  return l_ret;
}

void reference_file::load_file() {
  MStatus l_status{};
  auto l_node = get_ref_node();
  if (l_node.isNull()) {
    default_logger_raw()->log(log_loc(), spdlog::level::err, "引用文件 {} 没有连接文件", get_namespace());
    return;
  }
  MString l_file_str = MFileIO::loadReferenceByNode(l_node, &l_status);
  maya_chick(l_status);
  default_logger_raw()->log(log_loc(), spdlog::level::info, "加载引用文件 {}", l_file_str);
}

MSelectionList reference_file::get_collision_model() const {
  MFnDependencyNode l_file_info{};
  MStatus l_status{};
  maya_chick(l_file_info.setObject(file_info_node_));

  auto l_plug_lists = l_file_info.findPlug("collision_objects", false, &l_status);
  maya_chick(l_status);

  const auto l_couts = l_plug_lists.evaluateNumElements(&l_status);
  MSelectionList l_list{};

  for (auto i = 0; i < l_couts; ++i) {
    auto l_plug = l_plug_lists.elementByPhysicalIndex(i, &l_status);
    maya_chick(l_status);
    auto l_collision_message_plug = l_plug.source(&l_status);
    maya_chick(l_status);

    maya_chick(l_list.add(l_collision_message_plug.node()));
    DOODLE_LOG_INFO("添加碰撞体: {}", get_node_full_name(l_collision_message_plug.node()));
  }

  return l_list;
}

std::string reference_file::get_namespace() const {
  auto l_n = get_file_namespace();
  return l_n;
}

bool reference_file::has_sim_assets_file(const std::map<std::string, FSys::path> &in_sim_file_map) const {
  FSys::path k_m_str{get_abs_path()};
  auto l_stem = k_m_str.stem().generic_string();
  if (l_stem.ends_with("_cloth")) {
    l_stem = l_stem.substr(0, l_stem.size() - 6);
  }
  auto k_vfx_path = fmt::format("{}_cloth{}", l_stem, k_m_str.extension().generic_string());
  if (!in_sim_file_map.contains(k_vfx_path)) {
    default_logger_raw()->log(log_loc(), level::err, "引用文件 {} 没有对应的资产文件", get_namespace());
    return false;
  }
  return true;
}

bool reference_file::replace_sim_assets_file(const std::map<std::string, FSys::path> &in_sim_file_map) {
  auto l_node = get_ref_node();
  if (l_node.isNull()) {
    default_logger_raw()->log(log_loc(), level::err, "引用文件 {} 没有连接文件", get_namespace());
    return false;
  }

  MStatus k_s{};
  MFnReference k_ref{};
  maya_chick(k_ref.setObject(l_node));

  /// \brief 检查各种必须属性
  if (!k_ref.isLoaded(&k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    DOODLE_LOG_WARN("引用没有加载, 跳过!");
    return false;
  }

  FSys::path k_m_str{get_abs_path()};
  DOODLE_MAYA_CHICK(k_s);
  auto k_vfx_path = fmt::format("{}_cloth{}", k_m_str.stem().generic_string(), k_m_str.extension().generic_string());
  DOODLE_LOG_INFO("推测资产路径 {}", k_vfx_path);
  if (!in_sim_file_map.contains(k_vfx_path)) {
    default_logger_raw()->log(log_loc(), level::err, "引用文件 {} 没有对应的资产文件", get_namespace());
    return false;
  }

  /// \brief 替换引用文件
  return replace_file(in_sim_file_map.at(k_vfx_path));
}
bool reference_file::replace_file(const FSys::path &in_handle) {
  auto l_node = get_ref_node();
  if (l_node.isNull()) {
    default_logger_raw()->log(log_loc(), level::err, "引用文件 {} 没有连接文件", get_namespace());
    return false;
  }
  if (!FSys::exists(in_handle)) {
    default_logger_raw()->log(log_loc(), level::err, "引用文件 {} 不存在", in_handle);
    return false;
  }

  struct search_file_info_t {
    FSys::path path;
  };

  search_file_info_t l_search_file_info{in_handle};
  MStatus k_s{};
  {
    maya_call_guard l_guard{MSceneMessage::addCheckReferenceCallback(
        MSceneMessage::kBeforeLoadReferenceCheck,
        [](bool *retCode, const MObject &referenceNode, MFileObject &file, void *clientData) {
          auto *self  = reinterpret_cast<search_file_info_t *>(clientData);
          auto l_path = self->path;
          MStatus k_s{};
          k_s = file.setRawFullName(conv::to_ms(self->path.generic_string()));
          DOODLE_MAYA_CHICK(k_s);
          *retCode = true;
        },
        &l_search_file_info
    )};

    std::string l_s = d_str{MFileIO::loadReferenceByNode(l_node, &k_s)};
    maya_chick(k_s);
    default_logger_raw()->log(log_loc(), level::info, "加载引用文件 {}", l_s);
  }
  return true;
}
void reference_file::rename_namespace(const std::string &in_name) {
  MStatus k_s{};
  auto l_name_d = in_name;
  for (int l_i = 1; l_i < 1000 && MNamespace::namespaceExists(d_str{l_name_d}); ++l_i) {
    l_name_d = fmt::format("{}{}", in_name, l_i);
  }
  default_logger_raw()->log(
      log_loc(), level::info, "确认名称空间 {} 开始重命名名称空间到 {}", get_namespace(), l_name_d
  );
  k_s = MNamespace::renameNamespace(d_str{get_namespace()}, d_str{l_name_d});
  DOODLE_MAYA_CHICK(k_s);
  doodle::maya_plug::set_attribute(file_info_node_, "reference_file_namespace", l_name_d);
}

bool reference_file::has_node(const MSelectionList &in_list) {
  if (get_namespace().empty()) return false;
  MStatus k_s{};
  MObject k_node{};
  auto k_objs = MNamespace::getNamespaceObjects(conv::to_ms(get_namespace()), false, &k_s);
  for (MItSelectionList k_iter{in_list, MFn::Type::kDependencyNode, &k_s}; !k_iter.isDone(); k_iter.next()) {
    k_s = k_iter.getDependNode(k_node);
    DOODLE_MAYA_CHICK(k_s);

    for (int l_i = 0; l_i < k_objs.length(); ++l_i) {
      if (k_objs[l_i] == k_node) return true;
    }
  }
  return false;
}

bool reference_file::has_node(const MObject &in_node) const {
  MStatus k_s{};
  auto k_objs = MNamespace::getNamespaceObjects(d_str{get_namespace()}, false, &k_s);
  for (int l_i = 0; l_i < k_objs.length(); ++l_i) {
    if (k_objs[l_i] == in_node) return true;
  }
  return false;
}

FSys::path reference_file::get_abs_path() const {
  MStatus k_s{};
  MFnDependencyNode l_file_info{};
  if (l_file_info.setObject(file_info_node_)) {
    FSys::path l_path{};
    auto l_file_path_plug = l_file_info.findPlug("reference_file_path", false, &k_s);
    maya_chick(k_s);
    auto l_file_path = l_file_path_plug.asString(&k_s);
    maya_chick(k_s);
    if (l_file_path.length() == 0) return {};
    l_path = boost::locale::conv::utf_to_utf<wchar_t>(l_file_path.asUTF8());
    DOODLE_MAYA_CHICK(k_s);
    return l_path;
  } else {
    default_logger_raw()->log(
        log_loc(), spdlog::level::warn, "引用文件 {} 没有文件信息节点(引用节点空)", get_namespace()
    );
    return {};
  }
}

MSelectionList reference_file::get_all_object() const {
  MStatus k_s{};
  MSelectionList l_select;
  auto l_r = MNamespace::getNamespaceObjects(conv::to_ms(get_namespace()), false, &k_s);
  DOODLE_MAYA_CHICK(k_s);
  for (std::uint32_t i = 0u; i < l_r.length(); ++i) {
    k_s = l_select.add(l_r[i], true);
    DOODLE_MAYA_CHICK(k_s);
  }
  return l_select;
}
std::optional<MDagPath> reference_file::export_group_attr() const {
  if (get_file_namespace().empty()) return {};

  MStatus k_s{};

  DOODLE_MAYA_CHICK(k_s);
  MSelectionList k_select{};
  MDagPath l_path;

  k_s = k_select.add(d_str{fmt::format("{}:{}", get_namespace(), "UE4")}, true);
  if (k_s) {
    k_s = k_select.getDagPath(0, l_path);
    DOODLE_MAYA_CHICK(k_s);
  } else {
    DOODLE_LOG_INFO("引用文件 {} 没有配置中指定的 {} 导出组", get_namespace(), "UE4");
  }
  return l_path.isValid() ? std::make_optional(l_path) : std::optional<MDagPath>{};
}

std::optional<MDagPath> reference_file::get_field_dag() const {
  MSelectionList l_select{};

  MFnDependencyNode l_file_info{};
  MStatus l_status{};
  maya_chick(l_file_info.setObject(file_info_node_));

  auto l_plug = l_file_info.findPlug("wind_field", false, &l_status);
  maya_chick(l_status);

  if (l_plug.isDestination(&l_status)) {
    maya_chick(l_status);

    auto l_node_plug = l_plug.source(&l_status);
    maya_chick(l_status);

    auto l_node = l_node_plug.node(&l_status);
    maya_chick(l_status);
    return get_dag_path(l_node);
  }

  return {};
}

std::vector<MDagPath> reference_file::get_alll_cloth_obj() const {
  std::vector<MDagPath> l_export_path{};
  MStatus l_status{};
  MFnDagNode l_child_dag{};
  auto l_root = export_group_attr();
  if (!l_root) return {};

  MObject l_export_group{l_root->node(&l_status)};
  maya_chick(l_status);
  for (auto &&[e, l_cloth] : g_reg()->view<cloth_interface>().each()) {
    if (l_cloth->get_namespace() == get_namespace()) {
      auto l_obj = l_cloth->get_shape().node();
      for (MItDependencyGraph l_it{
               l_obj, MFn::kMesh, MItDependencyGraph::Direction::kDownstream,
               MItDependencyGraph::Traversal::kDepthFirst, MItDependencyGraph::Level::kNodeLevel, &l_status
           };
           !l_it.isDone(); l_it.next()) {
        auto l_current_path = get_dag_path(get_transform(l_it.currentItem(&l_status)));
        maya_chick(l_status);
        l_status = l_child_dag.setObject(l_current_path);
        maya_chick(l_status);
        if (l_child_dag.hasParent(l_export_group)) {
          auto l_path = l_current_path;
          if (auto l_it_j = ranges::find_if(l_export_path, boost::lambda2::_1 == l_path);
              l_it_j == l_export_path.end()) {
            l_export_path.emplace_back(l_current_path);
          }
        }
      }
    }
  }
  return l_export_path;
}

std::vector<reference_file> reference_file_factory::create_ref() const {
  MStatus l_status{};
  std::vector<reference_file> l_ret{};
  for (MItDependencyNodes l_it{MFn::kPluginDependNode, &l_status}; !l_it.isDone(); l_it.next()) {
    MFnDependencyNode l_fn_node{l_it.thisNode(), &l_status};
    maya_chick(l_status);
    if (l_fn_node.typeId() == doodle_file_info::doodle_id) {
      reference_file l_ref{l_it.thisNode()};
      if (!l_ref.get_namespace().empty()) {
        default_logger_raw()->log(
            log_loc(), spdlog::level::info, "获得引用文件 {} {}", l_ref.get_abs_path(), l_ref.get_namespace()
        );
        l_ret.emplace_back(l_ref);
      }
    }
  }
  return l_ret;
}
std::vector<reference_file> reference_file_factory::create_ref(const MSelectionList &in_list) const {
  std::vector<reference_file> l_ret{};

  MStatus l_status{};
  for (MItDependencyNodes l_it{MFn::kPluginDependNode, &l_status}; !l_it.isDone(); l_it.next()) {
    MFnDependencyNode l_fn_node{l_it.thisNode(), &l_status};
    maya_chick(l_status);
    if (l_fn_node.typeId() == doodle_file_info::doodle_id) {
      reference_file l_ref{l_it.thisNode()};
      MString l_name = get_plug(l_it.thisNode(), "reference_file_namespace").asString(&l_status);
      if (!l_ref.get_namespace().empty() && l_ref.has_node(in_list)) {
        default_logger_raw()->log(log_loc(), spdlog::level::info, "获得引用文件 {}", l_ref.get_abs_path());
        l_ret.emplace_back(l_ref);
      }
    }
  }

  return l_ret;
}

}  // namespace doodle::maya_plug
