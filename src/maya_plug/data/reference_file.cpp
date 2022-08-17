//
// Created by TD on 2021/11/30.
//

#include "reference_file.h"

#include <doodle_core/metadata/metadata.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/shot.h>
#include <doodle_core/metadata/export_file_info.h>
#include <doodle_core/metadata/redirection_path_info.h>

#include <maya/MDagPath.h>
#include <maya/MFileIO.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnReference.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MItSelectionList.h>
#include <maya/MObjectArray.h>
#include <maya/MPlug.h>
#include <maya/MTime.h>
#include <maya/MUuid.h>
#include <maya/MItDag.h>
#include <maya/MNamespace.h>
#include <maya/MFileObject.h>
#include <maya/MSceneMessage.h>
#include <maya/MItDependencyGraph.h>

#include <maya_plug/data/maya_file_io.h>
#include <maya_plug/data/qcloth_shape.h>
#include <main/maya_plug_fwd.h>
#include <maya_plug/data/find_duplicate_poly.h>
#include <maya_plug/data/maya_call_guard.h>

namespace doodle::maya_plug {
reference_file::reference_file()
    : path(),
      use_sim(false),
      collision_model(),
      p_m_object(),
      file_namespace(){};

reference_file::reference_file(
    const std::string &in_maya_namespace)
    : reference_file() {
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

void reference_file::chick_mobject() const {
  DOODLE_MAYA_CHICK(!file_namespace.empty(), doodle_error{"名称空间为空"});
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
  for (MItSelectionList l_it{get_collision_model(), MFn::Type::kMesh, &k_s};
       !l_it.isDone(&k_s);
       l_it.next()) {
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
  DOODLE_MAYA_CHICK(!file_namespace.empty(), doodle_error{"名称空间为空"});
  return file_namespace;
}

bool reference_file::replace_sim_assets_file() {
  if (!use_sim) {
    DOODLE_LOG_WARN("跳过不解算的文件 {}", path);
    return false;
  }

  chick_mobject();

  DOODLE_MAYA_CHICK(this->find_ref_node(), doodle_error{"缺失引用"});
  MFnReference k_ref{p_m_object};
  MStatus k_s{};

  /// \brief 检查各种必须属性
  if (!k_ref.isLoaded(&k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    DOODLE_LOG_WARN("引用没有加载, 跳过!");
    return false;
  }

  auto &k_cfg = g_reg()->ctx().at<project_config::base_config>();
  FSys::path k_m_str{get_path()};
  DOODLE_MAYA_CHICK(k_s);
  auto k_vfx_path = k_cfg.vfx_cloth_sim_path / fmt::format("{}_cloth{}", k_m_str.stem().generic_string(), k_m_str.extension().generic_string());
  DOODLE_LOG_INFO("推测资产路径 {}", k_vfx_path);
  if (!FSys::exists(k_vfx_path))
    return false;

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
        this)};

    std::string l_s = d_str{MFileIO::loadReferenceByNode(p_m_object, &k_s)};
    DOODLE_LOG_INFO("替换完成引用文件 {}", l_s);
  }
  return true;
}

bool reference_file::rename_material() const {
  chick_mobject();
  MStatus k_s{};
  MObjectArray k_list = MNamespace::getNamespaceObjects(d_str{file_namespace}, false, &k_s);
  MFnDependencyNode k_node;
  for (auto i = 0; i < k_list.length(); ++i) {
    auto k_obj = k_list[i];
    if (k_obj.hasFn(MFn::Type::kShadingEngine)) {  /// \brief 找到符合的着色集
      k_node.setObject(k_obj);
      auto k_plug = k_node.findPlug(d_str{"surfaceShader"}, true, &k_s);
      DOODLE_MAYA_CHICK(k_s);
      MPlugArray l_m_plug_array{};
      auto k_source = k_plug.source(&k_s);
      DOODLE_MAYA_CHICK(k_s);
      if (k_source.isNull(&k_s)) {
        continue;
      }
      DOODLE_MAYA_CHICK(k_s);
      auto k_mat = k_source.node(&k_s);  /// \brief 从属性链接获得材质名称
      DOODLE_MAYA_CHICK(k_s);
      MFnDependencyNode k_mat_node{};
      k_mat_node.setObject(k_mat);
      std::string k_mat_node_name = d_str{k_mat_node.name(&k_s)};
      DOODLE_MAYA_CHICK(k_s);
      /// \brief 重命名材质名称
      k_mat_node.setName(d_str{fmt::format("{}_mat", k_mat_node_name)}, false, &k_s);
      DOODLE_MAYA_CHICK(k_s);
      DOODLE_LOG_INFO("重命名材质 {} -> {}", d_str{k_node.name()}.str(), k_mat_node_name);

      k_node.setName(d_str{k_mat_node_name}, false, &k_s);
    }
  }

  return true;
}

FSys::path reference_file::export_abc(const MTime &in_start, const MTime &in_endl) const {
  MSelectionList k_select{};
  MStatus k_s{};
  auto &k_cfg = g_reg()->ctx().at<project_config::base_config>();
  try {
    k_s = k_select.add(d_str{fmt::format("{}:*{}", get_namespace(), k_cfg.export_group)}, true);
    DOODLE_MAYA_CHICK(k_s);
  } catch (const maya_InvalidParameter &err) {
    DOODLE_LOG_WARN("没有物体被配置文件中的 export_group 值选中, 不符合配置的文件, 不进行导出")
    return {};
  }

  /// \brief 进行dag遍历提取需要的节点
  {
    MDagPath k_root{};
    k_s = k_select.getDagPath(0, k_root);
    DOODLE_MAYA_CHICK(k_s);
    MItDag k_it{};
    k_s = k_it.reset(k_root, MItDag::kDepthFirst, MFn::Type::kMesh);
    DOODLE_MAYA_CHICK(k_s);
    MFnDagNode l_fn_dag_node{};
    k_select.clear();
    for (; !k_it.isDone(&k_s); k_it.next()) {
      DOODLE_MAYA_CHICK(k_s);
      k_s = k_it.getPath(k_root);
      DOODLE_MAYA_CHICK(k_s);

      k_s = l_fn_dag_node.setObject(k_root);
      DOODLE_MAYA_CHICK(k_s);
      /// \brief 检查一下是否是中间对象
      if (!l_fn_dag_node.isIntermediateObject(&k_s)) {
        DOODLE_MAYA_CHICK(k_s)
        k_s = k_select.add(l_fn_dag_node.object());
        DOODLE_MAYA_CHICK(k_s);
      }
    }
  }

  return export_abc(in_start, in_endl, k_select);
}
bool reference_file::add_collision() const {
  if (collision_model.empty())
    return true;

  MStatus k_s{};
  k_s = MGlobal::executeCommand(d_str{R"(lockNode -l false -lu false ":initialShadingGroup";)"});
  DOODLE_MAYA_CHICK(k_s);

  auto l_item = this->get_collision_model();
  k_s         = l_item.add(d_str{fmt::format("{}:qlSolver1", get_namespace())}, true);
  DOODLE_MAYA_CHICK(k_s);
  k_s = MGlobal::setActiveSelectionList(l_item);
  DOODLE_MAYA_CHICK(k_s);
  k_s = MGlobal::executeCommand(d_str{"qlCreateCollider;"});
  DOODLE_MAYA_CHICK(k_s);
  return true;
}

FSys::path reference_file::export_fbx(const MTime &in_start, const MTime &in_end) const {
  MSelectionList k_select{};
  MStatus k_s{};
  auto &k_cfg = g_reg()->ctx().at<project_config::base_config>();
  try {
    k_s = k_select.add(d_str{fmt::format("{}:*{}", get_namespace(), k_cfg.export_group)}, true);
    DOODLE_MAYA_CHICK(k_s);
  } catch (const maya_InvalidParameter &err) {
    DOODLE_LOG_WARN("没有物体被配置文件中的 export_group 值选中, 疑似场景文件, 或为不符合配置的文件, 不进行导出")
    return {};
  }

  return export_fbx(in_start, in_end, k_select);
}
bool reference_file::has_node(const MSelectionList &in_list) {
  chick_mobject();
  MStatus k_s{};
  MObject k_node{};
  for (MItSelectionList k_iter{in_list, MFn::Type::kDependencyNode, &k_s};
       !k_iter.isDone();
       k_iter.next()) {
    k_s = k_iter.getDependNode(k_node);
    DOODLE_MAYA_CHICK(k_s);

    if (has_node(k_node))
      return true;
  }
  return false;
}

bool reference_file::has_node(const MObject &in_node) const {
  chick_mobject();
  MStatus k_s{};
  auto k_objs = MNamespace::getNamespaceObjects(d_str{file_namespace}, false, &k_s);
  for (int l_i = 0; l_i < k_objs.length(); ++l_i) {
    if (k_objs[l_i] == in_node)
      return true;
  }

  return false;
}
bool reference_file::is_loaded() const {
  chick_mobject();
  ///@brief  引用为空的情况下，我们主动测试一下是否有导出组，如果有就可以认为时已加载的
  try {
    MFnReference k_ref{p_m_object};
    MStatus k_s{};
    auto k_r = k_ref.isLoaded(&k_s);
    DOODLE_MAYA_CHICK(k_s);
    return k_r;
  } catch (const maya_error &inerr) {
    DOODLE_LOG_INFO("查询引用方法 {} 错误, 使用寻找配置导出组的方式确认 ", inerr.what());
    return has_ue4_group();
  }
}
bool reference_file::has_sim_cloth() {
  chick_mobject();
  MStatus k_s{};
  MObjectArray k_objs = MNamespace::getNamespaceObjects(d_str{file_namespace}, false, &k_s);
  DOODLE_CHICK(k_s);
  MFnDependencyNode k_node{};
  for (int l_i = 0; l_i < k_objs.length(); ++l_i) {
    k_s = k_node.setObject(k_objs[l_i]);
    DOODLE_CHICK(k_s);
    if (k_node.typeName(&k_s) == "qlSolverShape") {
      DOODLE_CHICK(k_s);
      return true;
    }
  }
  return false;
}
bool reference_file::set_namespace(const std::string &in_namespace) {
  DOODLE_CHICK(!in_namespace.empty(), doodle_error{"空名称空间"});
  file_namespace = in_namespace.substr(1);
  find_ref_node();
  return has_ue4_group();
}
bool reference_file::find_ref_node() {
  chick_mobject();
  if (!p_m_object.isNull())
    return true;

  MStatus k_s;
  MFnReference k_file;
  DOODLE_LOG_INFO("名称空间 {} 开始寻找的引用", file_namespace);
  for (MItDependencyNodes refIter(MFn::kReference); !refIter.isDone(); refIter.next()) {
    k_s = k_file.setObject(refIter.thisNode());
    DOODLE_CHICK(k_s);
    const auto &&k_mata_str = k_file.associatedNamespace(false, &k_s);
    if (k_mata_str == file_namespace.c_str()) {
      p_m_object = refIter.thisNode();
    }
  }
  if (p_m_object.isNull()) {
    DOODLE_LOG_INFO("名称空间 {} 没有引用文件", file_namespace);
    return false;
  }

  MFnReference k_ref{p_m_object, &k_s};
  DOODLE_CHICK(k_s);
  path = d_str{k_ref.fileName(false, true, true, &k_s)};
  DOODLE_LOG_INFO("获得引用路径 {} 名称空间 {}", path, file_namespace);
  return true;
}
bool reference_file::has_ue4_group() const {
  chick_mobject();
  MStatus k_s{};
  MObjectArray k_objs = MNamespace::getNamespaceObjects(d_str{file_namespace}, false, &k_s);
  DOODLE_CHICK(k_s);
  MSelectionList k_select{};
  auto &k_cfg = g_reg()->ctx().at<project_config::base_config>();
  try {
    k_s = k_select.add(d_str{fmt::format("{}:*{}", get_namespace(), k_cfg.export_group)}, true);
    DOODLE_CHICK(k_s);
    return true;
  } catch (const maya_InvalidParameter &err) {
    DOODLE_LOG_INFO("引用文件 {} 没有配置中指定的 {} 导出组", get_namespace(), k_cfg.export_group);
    return false;
  }
}
void reference_file::qlUpdateInitialPose() const {
  DOODLE_LOG_INFO("开始更新解算文件 {} 中的布料初始化姿势 {}", get_namespace());
  MStatus l_status{};
  auto l_v = find_duplicate_poly{}(
      MNamespace::getNamespaceObjects(d_str{this->get_namespace()}, false, &l_status));
  DOODLE_CHICK(l_status);

  for (auto &&[l_obj1, l_obj2] : l_v) {
    MSelectionList l_list{};
    DOODLE_CHICK(l_list.add(l_obj1));
    DOODLE_CHICK(l_list.add(l_obj2));
    DOODLE_CHICK(MGlobal::setActiveSelectionList(l_list));
    DOODLE_CHICK(
        MGlobal::executeCommand(
            d_str{
                "qlUpdateInitialPose;"}));
  }
}
entt::handle reference_file::export_file(const reference_file::export_arg &in_arg) {
  entt::handle out_{};
  FSys::path l_path{};

  export_file_info::export_type l_type{};
  switch (in_arg.export_type_p) {
    case export_type::abc: {
      l_type = export_file_info::export_type::abc;
      l_path = export_abc(in_arg.start_p, in_arg.end_p);

    } break;
    case export_type::fbx: {
      l_type = export_file_info::export_type::fbx;
      l_path = export_fbx(in_arg.start_p, in_arg.end_p);
    } break;
  }
  if (!l_path.empty()) {
    out_ = make_handle();
    FSys::path l_ref_file{this->path};
    if (l_ref_file.empty()) {
      l_ref_file = this->get_namespace();
    }
    auto l_eps{""s};
    if (episodes::analysis_static(out_, l_path))
      l_eps = fmt::to_string(out_.get<episodes>());
    shot::analysis_static(out_, l_path);
    auto l_upload_prefix = FSys::path{magic_enum::enum_name(l_type).data()} / l_eps;
    out_.emplace<export_file_info>(l_path,
                                   boost::numeric_cast<std::int32_t>(in_arg.start_p.value()),
                                   boost::numeric_cast<std::int32_t>(in_arg.end_p.value()),
                                   l_ref_file,
                                   l_type)
        .upload_path_ = l_upload_prefix;
    export_file_info::write_file(out_);
  }
  return out_;
}

entt::handle reference_file::export_file_select(
    const reference_file::export_arg &in_arg,
    const MSelectionList &in_list) {
  entt::handle out_{};
  FSys::path l_path{};

  export_file_info::export_type l_type{};
  switch (in_arg.export_type_p) {
    case export_type::abc: {
      l_type = export_file_info::export_type::abc;
      l_path = export_abc(in_arg.start_p, in_arg.end_p, in_list);

    } break;
    case export_type::fbx: {
      l_type = export_file_info::export_type::fbx;
      l_path = export_fbx(in_arg.start_p, in_arg.end_p, in_list);
    } break;
  }
  if (!l_path.empty()) {
    out_ = make_handle();
    FSys::path l_ref_file{this->path};
    if (l_ref_file.empty()) {
      l_ref_file = this->get_namespace();
    }
    auto l_eps{""s};
    if (episodes::analysis_static(out_, l_path))
      l_eps = fmt::to_string(out_.get<episodes>());
    auto l_upload_prefix = FSys::path{magic_enum::enum_name(l_type).data()} / l_eps;
    shot::analysis_static(out_, l_path);
    out_.emplace<export_file_info>(l_path,
                                   boost::numeric_cast<std::int32_t>(in_arg.start_p.value()),
                                   boost::numeric_cast<std::int32_t>(in_arg.end_p.value()),
                                   l_ref_file,
                                   l_type)
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
            DOODLE_CHICK(k_s);
            *retCode = FSys::exists(self->path);
          } else {
            *retCode = false;
          }
        },
        this)};

    std::string l_s = d_str{MFileIO::loadReferenceByNode(p_m_object, &k_s)};
    DOODLE_CHICK(k_s);
    DOODLE_LOG_INFO("替换完成引用文件 {}", l_s);
  }
  auto l_name   = get_path().stem().generic_string();
  auto l_name_d = l_name;
  for (int l_i = 1; l_i < 1000 && MNamespace::namespaceExists(d_str{l_name_d}); ++l_i) {
    l_name_d = fmt::format("{}{}", l_name, l_i);
  }
  DOODLE_LOG_INFO("确认名称空间 {}", l_name_d);

  DOODLE_LOG_INFO("开始重命名名称空间 {} 到 {}", get_namespace(), l_name_d);
  k_s = MNamespace::renameNamespace(d_str{get_namespace()}, d_str{l_name_d});
  DOODLE_CHICK(k_s);
  file_namespace = l_name_d;
  DOODLE_CHICK(find_ref_node(), doodle_error{"没有在新的名称空间中查询到引用节点"});
  DOODLE_CHICK(has_ue4_group(), doodle_error{"没有在引用文件中找到 导出 组"});
  return false;
}
FSys::path reference_file::get_path() const {
  MStatus k_s{};
  MFnReference k_ref{p_m_object, &k_s};
  DOODLE_CHICK(k_s);
  FSys::path l_path = d_str{k_ref.fileName(true, true, false, &k_s)}.str();
  DOODLE_CHICK(k_s);
  return l_path;
}
FSys::path reference_file::get_abs_path() const {
  MStatus k_s{};
  MFnReference k_ref{p_m_object, &k_s};
  DOODLE_CHICK(k_s);
  FSys::path l_path = d_str{k_ref.fileName(false, false, false, &k_s)}.str();
  DOODLE_CHICK(k_s);
  return l_path;
}
FSys::path reference_file::export_abc(const MTime &in_start, const MTime &in_end, const MSelectionList &in_export_obj) const {
  FSys::path out_{};
  rename_material();
  MSelectionList k_select{in_export_obj};
  MStatus k_s{};
  auto &k_cfg = g_reg()->ctx().at<project_config::base_config>();

  if (k_select.isEmpty()) {
    DOODLE_LOG_INFO("没有找到合并对象")
    return out_;
  }
  /// \brief 进行dag遍历提取需要的节点

  if (k_select.length(&k_s) > 1) {
    DOODLE_CHICK(k_s);
    MStringArray k_mearge_names{};
    k_s = k_select.getSelectionStrings(k_mearge_names);
    DOODLE_CHICK(k_s);
    std::vector<std::string> l_names;
    for (int l_i = 0; l_i < k_mearge_names.length(); ++l_i) {
      l_names.emplace_back(d_str{k_mearge_names[l_i]});
    }

    MStringArray k_r_s{};
    auto k_name       = fmt::format("{}_export_abc", get_namespace());
    std::string l_mel = fmt::format(R"(polyUnite -ch 1 -mergeUVSets 1 -centerPivot -name "{}" {};)",
                                    k_name,
                                    fmt::join(l_names, " "));
    DOODLE_LOG_INFO("开始合并网格体 {}", fmt::join(l_names, " "));
    k_s = MGlobal::executeCommand(d_str{l_mel},
                                  k_r_s,
                                  true);
    DOODLE_CHICK(k_s);

    k_select.clear();
    k_s = k_select.add(k_r_s[0], true);
    DOODLE_CHICK(k_s);
  }

  if (k_select.isEmpty()) {
    DOODLE_LOG_INFO("没有找到合并对象")
    return out_;
  }

  MDagPath k_mesh_path{};
  k_s = k_select.getDagPath(0, k_mesh_path);
  DOODLE_CHICK(k_s);

  auto k_seance_name = maya_file_io::get_current_path().stem().generic_string();
  auto k_path        = maya_file_io::work_path(fmt::format("abc/{}", k_seance_name));

  if (!exists(k_path)) {
    create_directories(k_path);
  }
  k_path /= fmt::format("{}_{}_{}-{}.abc", k_seance_name, get_namespace(), in_start.as(MTime::uiUnit()), in_end.as(MTime::uiUnit()));

  /// \brief 导出abc命令
  k_s = MGlobal::executeCommand(d_str{
      fmt::format(R"(
AbcExport -j "-frameRange {} {} -stripNamespaces -uvWrite -writeFaceSets -worldSpace -dataFormat ogawa -root {} -file {}";
)",
                  in_start.as(MTime::uiUnit()),                 /// \brief 开始时间
                  in_end.as(MTime::uiUnit()),                   /// \brief 结束时间
                  d_str{k_mesh_path.fullPathName(&k_s)}.str(),  /// \brief 导出物体的根路径
                  k_path.generic_string())});                   /// \brief 导出文件路径，包含文件名和文件路径
  DOODLE_CHICK(k_s);
  return k_path;
}
FSys::path reference_file::export_fbx(const MTime &in_start, const MTime &in_end, const MSelectionList &in_export_obj) const {
  FSys::path out_{};

  DOODLE_CHICK(is_loaded(), doodle_error{"需要导出fbx的引用必须加载"});

  MStatus k_s{};
  auto &k_cfg = g_reg()->ctx().at<project_config::base_config>();

  if (in_export_obj.isEmpty()) {
    DOODLE_LOG_WARN("没有选中的物体, 不进行输出")
    return out_;
  }

  k_s = MGlobal::setActiveSelectionList(in_export_obj);
  DOODLE_CHICK(k_s);

  auto k_file_path = maya_file_io::work_path("fbx") / maya_file_io::get_current_path().stem();

  if (!FSys::exists(k_file_path))
    FSys::create_directories(k_file_path);
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
  auto l_comm = fmt::format(maya_bakeResults_str,
                            in_start.value(), in_end.value(), "false"s, get_namespace(), k_cfg.export_group);
  DOODLE_LOG_INFO("开始使用命令 {} 主动烘培动画帧", l_comm);
  try {
    k_s = MGlobal::executeCommand(d_str{l_comm});
    DOODLE_CHICK(k_s);
  } catch (const maya_Failure &in) {
    DOODLE_LOG_INFO("开始主动烘培动画帧失败, 开始使用备用参数重试 {}", in.what());
    try {
      l_comm = fmt::format(maya_bakeResults_str,
                           in_start.value(), in_end.value(), "true"s, get_namespace(), k_cfg.export_group);
      DOODLE_LOG_INFO("开始使用命令 {} 主动烘培动画帧", l_comm);
      k_s = MGlobal::executeCommand(d_str{l_comm});
      DOODLE_CHICK(k_s);
    } catch (const maya_Failure &in2) {
      DOODLE_LOG_INFO("开始主动烘培动画帧失败, 开始使用默认参数重试  error {} ", in2.what());

      try {
        l_comm = fmt::format(R"(bakeResults  -simulation true -t "{}:{}" -hierarchy below "{}:*{}";)", in_start.value(), in_end.value(), get_namespace(), k_cfg.export_group);
        DOODLE_LOG_INFO("开始使用命令 {} 主动烘培动画帧", l_comm);
        k_s = MGlobal::executeCommand(d_str{l_comm});
        DOODLE_CHICK(k_s);
      } catch (const maya_Failure &in3) {
        DOODLE_LOG_INFO("烘培失败, 直接导出 {}", in3.what());
      }
    }

    DOODLE_LOG_INFO("完成烘培, 不检查结果, 直接进行输出");
  }

  k_file_path /= fmt::format("{}_{}_{}-{}.fbx",
                             maya_file_io::get_current_path().stem().generic_string(),
                             get_namespace(),
                             in_start.value(),
                             in_end.value());
  DOODLE_LOG_INFO("导出fbx文件路径 {}", k_file_path);

  auto k_comm = fmt::format("FBXExportBakeComplexStart -v {};", in_start.value());
  k_s         = MGlobal::executeCommand(d_str{k_comm});
  DOODLE_CHICK(k_s);

  k_comm = fmt::format("FBXExportBakeComplexEnd -v {};", in_end.value());
  k_s    = MGlobal::executeCommand(d_str{k_comm});
  DOODLE_CHICK(k_s);

  k_comm = std::string{"FBXExportBakeComplexAnimation -v true;"};
  k_s    = MGlobal::executeCommand(d_str{k_comm});
  DOODLE_CHICK(k_s);

  k_comm = std::string{"FBXExportConstraints -v true;"};
  k_s    = MGlobal::executeCommand(d_str{k_comm});
  DOODLE_CHICK(k_s);

  k_comm = fmt::format(R"(FBXExport -f "{}" -s;)", k_file_path.generic_string());
  k_s    = MGlobal::executeCommand(d_str{k_comm});
  DOODLE_CHICK(k_s);
  return k_file_path;
}
MSelectionList reference_file::get_all_object() const {
  MStatus k_s{};
  MSelectionList l_select;
  auto l_r =
      MNamespace::getNamespaceObjects(d_str{file_namespace}, false, &k_s);
  DOODLE_CHICK(k_s);
  for (std::uint32_t i = 0u; i < l_r.length(); ++i) {
    k_s = l_select.add(l_r[i], true);
    DOODLE_CHICK(k_s);
  }
  return l_select;
}
std::optional<MDagPath> reference_file::export_group_attr() const {
  chick_mobject();
  MStatus k_s{};

  DOODLE_CHICK(k_s);
  MSelectionList k_select{};
  auto &k_cfg = g_reg()->ctx().at<project_config::base_config>();
  MDagPath l_path;
  try {
    k_s = k_select.add(d_str{fmt::format("{}:*{}", get_namespace(), k_cfg.export_group)}, true);
    DOODLE_CHICK(k_s);
    k_s = k_select.getDagPath(0, l_path);
    DOODLE_CHICK(k_s);
  } catch (const maya_InvalidParameter &err) {
    DOODLE_LOG_INFO("引用文件 {} 没有配置中指定的 {} 导出组", get_namespace(), k_cfg.export_group);
  }
  return l_path.isValid() ? std::make_optional(l_path) : std::optional<MDagPath>{};
}
std::vector<MDagPath> reference_file::qcloth_export_model() const {
  auto l_cloth = qcloth_shape::create(make_handle(*this));

  MStatus l_status{};
  MObject l_return{};
  std::vector<MDagPath> l_all_path{};
  if (!has_ue4_group())
    return l_all_path;

  MFnDagNode l_child{};
  MObject l_export_group{export_group_attr()->node(&l_status)};
  DOODLE_CHICK(l_status);

  for (auto &&qlc : l_cloth) {
    auto l_object = qlc.get<qcloth_shape>().ql_cloth_shape().node(&l_status);
    DOODLE_CHICK(l_status);
    for (
        MItDependencyGraph l_it{l_object,
                                MFn::Type::kMesh,
                                MItDependencyGraph::Direction::kDownstream,
                                MItDependencyGraph::Traversal::kDepthFirst,
                                MItDependencyGraph::Level::kNodeLevel,
                                &l_status};
        !l_it.isDone() && l_status;
        l_it.next()) {
      auto l_temp_sp = get_dag_path(l_it.currentItem(&l_status));
      DOODLE_CHICK(l_status);
      auto l_current_path = get_dag_path(l_temp_sp.transform(&l_status));
      DOODLE_CHICK(l_status);
      l_status = l_child.setObject(l_current_path);
      DOODLE_CHICK(l_status);
      if (l_child.hasParent(l_export_group)) {
        auto l_path = l_current_path;
        if (auto l_it_j = ranges::find_if(l_all_path, [&](const MDagPath &in) { return l_path == in; });
            l_it_j == l_all_path.end()) {
          l_all_path.emplace_back(l_current_path);
        }
      }
    }
  }

  return l_all_path;
}

}  // namespace doodle::maya_plug
