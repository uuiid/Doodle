//
// Created by TD on 2021/12/6.
//

#include "qcloth_shape.h"
#include "doodle_lib/metadata/metadata.h"

#include <doodle_lib/metadata/project.h>

#include <maya/MDagPath.h>
#include <maya/MFileIO.h>
#include <maya/MFnMesh.h>
#include <maya/MPlug.h>
#include <maya/MNamespace.h>
#include <maya/MItDag.h>
#include <maya/MDagModifier.h>
#include <maya/MFnSet.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MFnSkinCluster.h>

#include <maya_plug/data/reference_file.h>
#include <maya_plug/data/maya_file_io.h>
#include <maya_plug/maya_plug_fwd.h>

#include <magic_enum.hpp>

namespace doodle::maya_plug {

namespace qcloth_shape_n {
maya_obj::maya_obj() = default;
maya_obj::maya_obj(const MObject& in_object) {
  obj = in_object;
  MStatus k_s;
  MFnDependencyNode k_node{in_object, &k_s};
  DOODLE_CHICK(k_s);
  p_name = d_str{k_node.name(&k_s)};
  DOODLE_CHICK(k_s);
}
}  // namespace qcloth_shape_n

namespace {
MObject make_low_node(const MObject& in_object, const MObject& in_parent) {
  MStatus l_s{};
  MFnDagNode l_node{};
  /// \brief 返回的结果, 复制出来的obj
  MObject l_r{};

  // 复制传入节点
  l_s = l_node.setObject(in_object);
  DOODLE_CHICK(l_s);
  l_r = l_node.duplicate(false, false, &l_s);
  chick_true<maya_error>(!l_r.isNull(), DOODLE_SOURCE_LOC, "无法复制节点");

  // 设置复制节点的名称
  string k_anim_mesh_name = d_str{l_node.name(&l_s)};
  DOODLE_CHICK(l_s);
  l_node.setName(d_str{fmt::format("{}_input", k_anim_mesh_name)}, false, &l_s);
  DOODLE_CHICK(l_s);

  // 设置材质属性
  MFnSet l_mat{get_shading_engine(in_object), &l_s};
  DOODLE_CHICK(l_s);
  l_s = l_mat.addMember(l_r);
  DOODLE_CHICK(l_s);

  /// 将复制节点添加为父节点的子物体
  l_s = l_node.setObject(in_parent);
  DOODLE_CHICK(l_s);
  l_s = l_node.addChild(l_r);
  DOODLE_CHICK(l_s);
  return l_r;
}

std::vector<MObject> make_high_node(const qcloth_shape_n::shape_list& in_high_node, const MObject& in_parent) {
  MStatus l_s{};
  std::vector<MObject> l_r{};
  MFnDagNode l_node{};
  /// 复制高模节点作为输出
  std::transform(in_high_node.begin(), in_high_node.end(),
                 std::back_inserter(l_r),
                 [&](const qcloth_shape_n::maya_obj& in_object) -> MObject {
                   /// 复制模型
                   l_s = l_node.setObject(in_object.obj);
                   DOODLE_CHICK(l_s);
                   auto l_r = l_node.duplicate(false, false, &l_s);
                   DOODLE_CHICK(l_s);

                   // 设置复制节点的名称
                   string k_anim_mesh_name = d_str{l_node.name(&l_s)};
                   DOODLE_CHICK(l_s);
                   l_node.setName(d_str{fmt::format("{}_out_mesh", k_anim_mesh_name)}, false, &l_s);
                   DOODLE_CHICK(l_s);

                   /// 设置材质
                   MFnSet l_mat{get_shading_engine(in_object.obj), &l_s};
                   DOODLE_CHICK(l_s);
                   l_s = l_mat.addMember(l_r);
                   DOODLE_CHICK(l_s);

                   /// 将复制节点添加为父节点的子物体
                   l_s = l_node.setObject(in_parent);
                   DOODLE_CHICK(l_s);
                   l_s = l_node.addChild(l_r);
                   DOODLE_CHICK(l_s);

                   return l_r;
                 });
  return l_r;
}

void warp_model(const MObject& in_low, const std::vector<MObject>& in_high_node) {
  MStatus l_s{};

  /// 创建包裹变形(maya的包裹变形需要先选择高模, 可以多个, 然后选中低模) 包裹时需要添加独占式绑定参数
  MSelectionList k_select{};
  /// 添加高模
  std::for_each(in_high_node.begin(), in_high_node.end(),
                [&](const MObject& in_obj) -> void {
                  l_s = k_select.add(in_obj, false);
                  DOODLE_CHICK(l_s);
                });

  l_s = k_select.add(in_low, false);
  DOODLE_CHICK(l_s);
  /// 设置选择
  l_s = MGlobal::setActiveSelectionList(k_select);
  DOODLE_CHICK(l_s);
  l_s = MGlobal::executeCommand(d_str{R"(doWrapArgList "4" {"1","0","0.1", "1","1"};)"});
  DOODLE_CHICK(l_s);
}

void transfer_dynamic(const MObject& in_sim_node, const MObject& in_anim_node) {
  MStatus l_s{};
  MObject l_skin_cluster{};
  MFnDagNode l_node{};
  /// 寻找高模的皮肤簇
  auto l_shape = get_shape(in_anim_node);
  for (MItDependencyGraph i{l_shape,
                            MFn::kSkinClusterFilter,
                            MItDependencyGraph::Direction::kUpstream};
       !i.isDone();
       i.next()) {
    l_skin_cluster = i.currentItem(&l_s);
    DOODLE_CHICK(l_s);
  }
  chick_true<maya_error>(!l_skin_cluster.isNull(), DOODLE_SOURCE_LOC, "没有找到混合变形节点");
  /// 先将高模的皮肤簇权重重置为0;
  MFnSkinCluster l_fn_skin_cluster{l_skin_cluster, &l_s};
  l_s = l_fn_skin_cluster.setEnvelope(0);
  DOODLE_CHICK(l_s);

  ///  获得名称进行格式化命令
  MFnDagNode l_node1{in_anim_node, &l_s};
  DOODLE_CHICK(l_s);
  string l_aim_name = d_str{l_node.name(&l_s)};
  DOODLE_CHICK(l_s);

  l_s = l_node.setObject(in_sim_node);
  DOODLE_CHICK(l_s);

  string l_sim_name = d_str{l_node.name(&l_s)};
  DOODLE_CHICK(l_s);
  /// 这个设置包裹
  MStringArray l_blend{};
  l_s = MGlobal::executeCommand(d_str{
                                    fmt::format(R"(blendShape -automatic {} {};)",
                                                l_sim_name, l_aim_name)},
                                l_blend, true);
  DOODLE_CHICK(l_s);
  //      /// 开始设置权重
  MGlobal::executeCommand(d_str{fmt::format(R"(setAttr "{}.{}" 1;)",
                                            l_blend[0], l_sim_name)});
};
/**
 * @brief 检查组节点名称
 * @param in_node 传入的节点
 * @param in_name 节点名称
 * @return 不符合名称的话返回空节点, 否则返回传入节点
 */
MObject chick_group(const MFnDagNode& in_node,
                    const string& in_name) {
  MStatus l_s{};
  if (in_node.name(&l_s) == d_str{in_name}) {
    DOODLE_CHICK(l_s);
    auto l_obj = in_node.object(&l_s);
    DOODLE_CHICK(l_s);
    return l_obj;
  }
  return {};
}

/**
 * @brief 创建组节点,并根据传入的名称和父对象进行设置
 * @param in_modifier 传入的管理器
 * @param in_name 传入的名称
 * @param in_parent 传入的父物体
 * @return 创建的maya组节点
 */
MObject make_group(MDagModifier& in_modifier,
                   const string& in_name,
                   const MObject& in_parent) {
  MStatus l_s{};
  auto l_r = in_modifier.createNode(d_str{"transform"}, in_parent, &l_s);
  DOODLE_CHICK(l_s);
  l_s = in_modifier.renameNode(l_r, d_str{in_name});
  DOODLE_CHICK(l_s);
  l_s = in_modifier.doIt();
  DOODLE_CHICK(l_s);
  return l_r;
}
}  // namespace

qcloth_shape::qcloth_shape() = default;

qcloth_shape::qcloth_shape(const entt::handle& in_ref_file, const MObject& in_object)
    : qcloth_shape() {
  p_ref_file = in_ref_file;
  obj        = in_object;
  chick_component<reference_file>(p_ref_file);
}
bool qcloth_shape::set_cache_folder() const {
  MStatus k_s{};
  /// \brief 获得解算节点fn
  MFnDependencyNode k_node{obj, &k_s};
  DOODLE_CHICK(k_s);
  string k_name{d_str{k_node.name(&k_s)}};
  DOODLE_CHICK(k_s);
  string k_namespace = p_ref_file.get<reference_file>().get_namespace();
  auto& k_cfg        = p_ref_file.get<reference_file>().get_prj().get<project::cloth_config>();

  DOODLE_CHICK(k_s);
  string k_node_name = d_str{MNamespace::stripNamespaceFromName(k_node.name(), &k_s)};
  DOODLE_CHICK(k_s);
  {
    auto k_cache = k_node.findPlug(d_str{"cacheFolder"}, false, &k_s);
    DOODLE_CHICK(k_s);
    auto k_file_name       = maya_file_io::get_current_path();
    /// \brief 使用各种信息确认缓存相对路径
    const string& l_string = fmt::format("cache/{}/{}/{}",
                                         k_file_name.stem().generic_string(),
                                         k_namespace,
                                         k_node_name);
    DOODLE_LOG_INFO("设置缓存路径 {}", l_string);
    /// \brief 删除已经缓存的目录
    auto k_path = maya_file_io::work_path(l_string);
    if (FSys::exists(k_path)) {
      DOODLE_LOG_INFO("发现缓存目录, 主动删除 {}", k_path);
      FSys::remove_all(k_path);
    }
    FSys::create_directories(k_path);
    k_s = k_cache.setString(d_str{l_string});
    DOODLE_CHICK(k_s);
  }
  {
    auto k_cache = k_node.findPlug(d_str{"cacheName"}, true, &k_s);
    DOODLE_CHICK(k_s);
    k_cache.setString(d_str{k_node_name});
  }
  return true;
}

bool qcloth_shape::create_cache() const {
  if (obj.isNull())
    throw doodle_error{"空组件"};
  MStatus k_s{};
  /// 直接使用 MItDependencyGraph 搜素 kMesh 类型并同步
  MFnMesh k_shape{get_first_mesh(obj), &k_s};
  DOODLE_CHICK(k_s);
  k_s = k_shape.updateSurface();
  DOODLE_CHICK(k_s);
  k_s = k_shape.syncObject();
  DOODLE_CHICK(k_s);

  return true;
}

void qcloth_shape::create_sim_cloth(const entt::handle& in_handle) {
  chick_component<qcloth_shape_n::maya_obj, qcloth_shape_n::shape_list>(in_handle);
  chick_ctx<root_ref>();
  auto& k_ref  = g_reg()->ctx<root_ref>().root_handle().get<project::cloth_config>();
  auto l_group = get_cloth_group();

  MStatus k_s{};
  MFnDagNode l_node{};
  MDagModifier l_modifier{};

  /// \brief 主要的动画输出节点(需要输入到解算输入端)
  auto& k_anim_mesh      = in_handle.get<qcloth_shape_n::maya_obj>();
  /// \brief 主要的输入节点
  auto k_proxy_node      = make_low_node(k_anim_mesh.obj, l_group.anim_grp);
  /// \brief 动画高模
  auto& k_maya_high_mesh = in_handle.get<qcloth_shape_n::shape_list>();
  auto l_high_mesh       = make_high_node(k_maya_high_mesh, l_group.deform_grp);

  MDagPath l_path{};
  {  /// 创建动画网格和解算网络的输入
    /// 连接两个属性的输入和输出
    k_s = l_modifier.connect(get_plug(k_anim_mesh.obj, "outMesh"),
                             get_plug(k_proxy_node, "inMesh"));
    DOODLE_CHICK(k_s);
    k_s = l_modifier.doIt();
    DOODLE_CHICK(k_s);
  }
  {
    /// 创建解算网络
    MSelectionList l_selection_list{};
    k_s = l_selection_list.add(k_proxy_node);
    DOODLE_CHICK(k_s);
    k_s = MGlobal::setActiveSelectionList(l_selection_list);
    DOODLE_CHICK(k_s);
    k_s = MGlobal::executeCommand(d_str{"qlCreateCloth;"});
    DOODLE_CHICK(k_s);
  }
  {
    ///  这里由于是使用解算命令创建出来的， 所以需要寻找新的低模
    MObject k_low{};
    auto l_shape = get_shape(k_proxy_node);
    for (MItDependencyGraph i{l_shape, MFn::kMesh};
         !i.isDone();
         i.next()) {
      k_low = i.currentItem(&k_s);
      DOODLE_CHICK(k_s);
    }
    chick_true<maya_error>(!k_low.isNull(), DOODLE_SOURCE_LOC, "没有找到解算输出的模型");
    warp_model(k_low, l_high_mesh);
  }
  {
    /// 创建解算网络的输出 这个可以用融合变形(其中先选择主动变形物体, 再选择被变形物体)
    chick_true<maya_error>(l_high_mesh.size() == k_maya_high_mesh.size(), DOODLE_SOURCE_LOC, "节点数量不一致");
    for (int l_i = 0; l_i < l_high_mesh.size(); ++l_i) {
      transfer_dynamic(l_high_mesh[l_i], k_maya_high_mesh[l_i].obj);
    }
  }
}

qcloth_shape::cloth_group qcloth_shape::get_cloth_group() {
  MStatus k_s{};
  qcloth_shape::cloth_group k_r{};
  MFnDagNode k_node{};

  for (MItDag i{MItDag::kDepthFirst, MFn::Type::kTransform, &k_s}; !i.isDone(); i.next()) {
    DOODLE_CHICK(k_s);
    k_s = k_node.setObject(i.currentItem());
    DOODLE_CHICK(k_s);
    k_r.cfx_grp        = chick_group(k_node, "cfx_grp");
    k_r.solver_grp     = chick_group(k_node, "solver_grp");
    k_r.anim_grp       = chick_group(k_node, "anim_grp");
    k_r.constraint_grp = chick_group(k_node, "constraint_grp");
    k_r.collider_grp   = chick_group(k_node, "collider_grp");
    k_r.deform_grp     = chick_group(k_node, "deform_grp");
    k_r.export_grp     = chick_group(k_node, "export_grp");
  }
  MDagModifier k_m{};
  if (k_r.cfx_grp.isNull())
    k_r.cfx_grp = make_group(k_m, "cfx_grp", MObject::kNullObj);

  if (k_r.solver_grp.isNull())
    k_r.solver_grp = make_group(k_m, "solver_grp", k_r.solver_grp);

  if (k_r.anim_grp.isNull())
    k_r.anim_grp = make_group(k_m, "anim_grp", k_r.anim_grp);

  if (k_r.constraint_grp.isNull())
    k_r.constraint_grp = make_group(k_m, "constraint_grp", k_r.constraint_grp);

  if (k_r.collider_grp.isNull())
    k_r.collider_grp = make_group(k_m, "collider_grp", k_r.collider_grp);

  if (k_r.deform_grp.isNull())
    k_r.deform_grp = make_group(k_m, "deform_grp", k_r.deform_grp);

  if (k_r.export_grp.isNull())
    k_r.export_grp = make_group(k_m, "export_grp", k_r.export_grp);

  return k_r;
}

void qcloth_shape::add_child(const MObject& in_praent, MObject& in_child) {
  MStatus k_s{};
  MFnDagNode k_node{in_praent, &k_s};
  DOODLE_CHICK(k_s);
  k_s = k_node.addChild(in_child);
  DOODLE_CHICK(k_s);
}

}  // namespace doodle::maya_plug
