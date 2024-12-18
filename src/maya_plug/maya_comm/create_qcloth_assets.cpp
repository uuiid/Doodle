//
// Created by TD on 2022/7/20.
//

#include "create_qcloth_assets.h"

#include <maya_plug/data/qcloth_shape.h>

#include <maya/MAnimControl.h>
#include <maya/MArgDatabase.h>
#include <maya/MArgList.h>
#include <maya/MDagModifier.h>
#include <maya/MDagPath.h>
#include <maya/MFileIO.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSet.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MItMeshVertex.h>
#include <maya/MItSelectionList.h>
#include <maya/MNamespace.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
namespace doodle::maya_plug {
namespace create_qcloth_assets_ns {

constexpr char cloth[]       = {"-c"};
constexpr char cloth_l[]     = {"-cloth"};

constexpr char collision[]   = {"-co"};
constexpr char collision_l[] = {"-collision"};

constexpr char add[]         = {"-a"};
constexpr char add_l[]       = {"-add"};

constexpr char create[]      = {"-cr"};
constexpr char create_l[]    = {"-create"};

MSyntax syntax() {
  MSyntax l_syntax{};
  l_syntax.addFlag(cloth, cloth_l, MSyntax::MArgType::kString);
  l_syntax.addFlag(collision, collision_l, MSyntax::MArgType::kString);

  l_syntax.makeFlagMultiUse(cloth);
  l_syntax.makeFlagMultiUse(collision);
  l_syntax.enableQuery(false);
  l_syntax.enableEdit(false);
  return l_syntax;
}

class cloth_group {
 public:
  MObject cfx_grp;
  MObject solver_grp;
  MObject anim_grp;
  MObject constraint_grp;
  MObject collider_grp;
  MObject deform_grp;
  MObject export_grp;
  MObject deformBase_grp;
};

/**
 * @brief 获取传入动画节点(动画[绑定]网格体或者变换节点)链接的皮肤簇
 * @param in_anim_node 动画[绑定]网格体或者变换节点
 * @return 寻找到的皮肤簇(不为空)
 * @throw 为空时抛出异常 maya_error
 */
MObject get_skin_custer(const MObject& in_anim_node) {
  MStatus l_s{};
  MObject l_skin_cluster{};
  /// \brief 获得组件点上下文
  auto l_shape = maya_plug::get_shape(in_anim_node);

  /// 寻找高模的皮肤簇
  for (MItDependencyGraph i{l_shape, MFn::kSkinClusterFilter, MItDependencyGraph::Direction::kUpstream}; !i.isDone();
       i.next()) {
    l_skin_cluster = i.currentItem(&l_s);
    DOODLE_MAYA_CHICK(l_s);
  }

  DOODLE_CHICK(!l_skin_cluster.isNull(), doodle_error{"没有找到皮肤簇变形节点"s});
  return l_skin_cluster;
}

/**
 * @brief 复制并制作低模
 * @param in_object 传入的低模物体
 * @param in_parent 传入低模的父物体
 * @return 复制的低模
 */
MObject make_low_node(const MObject& in_object, const MObject& in_parent, const std::string& in_suffix) {
  MStatus l_s{};
  MFnDagNode l_node{};
  /// \brief 返回的结果, 复制出来的obj
  MObject l_r{};

  // 复制传入节点
  l_s = l_node.setObject(in_object);
  DOODLE_MAYA_CHICK(l_s);
  std::string k_anim_mesh_name = d_str{l_node.name(&l_s)};
  DOODLE_MAYA_CHICK(l_s);
  l_r = l_node.duplicate(false, false, &l_s);
  DOODLE_MAYA_CHICK(l_s);
  DOODLE_LOG_INFO("复制节点 {}", k_anim_mesh_name);

  // 设置复制节点的名称
  l_s = l_node.setObject(l_r);
  DOODLE_MAYA_CHICK(l_s);
  l_node.setName(d_str{fmt::format("{}_{}", k_anim_mesh_name, in_suffix)}, false, &l_s);
  DOODLE_MAYA_CHICK(l_s);
  DOODLE_LOG_INFO("设置复制节点名称 {}", l_node.name(&l_s));
  DOODLE_MAYA_CHICK(l_s);

  // 设置材质属性
  MFnSet l_mat{get_shading_engine(in_object), &l_s};
  DOODLE_MAYA_CHICK(l_s);
  l_s = l_mat.addMember(l_r);
  DOODLE_MAYA_CHICK(l_s);

  /// 将复制节点添加为父节点的子物体
  l_s = l_node.setObject(in_parent);
  DOODLE_MAYA_CHICK(l_s);
  l_s = l_node.addChild(l_r);
  DOODLE_MAYA_CHICK(l_s);
  DOODLE_LOG_INFO("设置复制节点父物体 {}", l_node.name(&l_s));
  DOODLE_MAYA_CHICK(l_s);
  return l_r;
}

/**
 * @brief 复制一次高模并将高模重新设置为父物体
 * @param in_high_node 传入的高模
 * @param in_parent 传入的父物体
 * @return 复制出来的高模列表
 */
std::vector<MObject> make_high_node(const std::vector<MObject>& in_list, const MObject& in_parent) {
  MStatus l_s{};
  std::vector<MObject> l_r{};
  MFnDagNode l_node{};
  /// 复制高模节点作为输出
  std::transform(in_list.begin(), in_list.end(), std::back_inserter(l_r), [&](const MObject& in_object) -> MObject {
    /// 复制模型
    l_s = l_node.setObject(in_object);
    DOODLE_MAYA_CHICK(l_s);
    std::string k_anim_mesh_name = d_str{l_node.name(&l_s)};
    DOODLE_MAYA_CHICK(l_s);
    auto l_r = l_node.duplicate(false, false, &l_s);
    DOODLE_MAYA_CHICK(l_s);
    DOODLE_LOG_INFO("复制高模节点 {}", k_anim_mesh_name);
    DOODLE_MAYA_CHICK(l_s);

    // 设置复制节点的名称
    l_node.setObject(l_r);
    l_node.setName(d_str{fmt::format("{}_out_mesh", k_anim_mesh_name)}, false, &l_s);
    DOODLE_MAYA_CHICK(l_s);
    DOODLE_LOG_INFO("设置复制高模节点名称 {}", l_node.name(&l_s));
    DOODLE_MAYA_CHICK(l_s);

    /// 设置材质
    MFnSet l_mat{get_shading_engine(in_object), &l_s};
    DOODLE_MAYA_CHICK(l_s);
    l_s = l_mat.addMember(l_r);
    DOODLE_MAYA_CHICK(l_s);

    /// 将复制节点添加为父节点的子物体
    l_s = l_node.setObject(in_parent);
    DOODLE_MAYA_CHICK(l_s);
    l_s = l_node.addChild(l_r);
    DOODLE_MAYA_CHICK(l_s);
    DOODLE_LOG_INFO("设置复制高模节点父物体 {}", l_node.name(&l_s));
    DOODLE_MAYA_CHICK(l_s);

    return l_r;
  });
  return l_r;
}
/**
 * @brief 将低模使用包裹变形包裹高模
 * @param in_low 传入的低模节点
 * @param in_high_node 传入的高模节点
 */
MObject warp_model(const MObject& in_low, const std::vector<MObject>& in_high_node) {
  MStatus l_s{};

  /// 创建包裹变形(maya的包裹变形需要先选择高模, 可以多个, 然后选中低模) 包裹时需要添加独占式绑定参数
  MSelectionList k_select{};
  MFnDependencyNode l_node{};
  std::string l_string{"添加 "};
  /// 添加高模
  std::for_each(in_high_node.begin(), in_high_node.end(), [&](const MObject& in_obj) -> void {
    l_s = k_select.add(in_obj, false);
    DOODLE_MAYA_CHICK(l_s);
    l_s = l_node.setObject(in_obj);
    DOODLE_MAYA_CHICK(l_s);
    l_string += fmt::format("高模节点 {}", l_node.name(&l_s));
    DOODLE_MAYA_CHICK(l_s);
  });

  l_s = k_select.add(in_low, false);
  DOODLE_MAYA_CHICK(l_s);
  l_s = l_node.setObject(in_low);
  DOODLE_MAYA_CHICK(l_s);
  l_string += fmt::format(" 低模节点 {} 进行包裹变形", l_node.name(&l_s));
  DOODLE_MAYA_CHICK(l_s);
  DOODLE_LOG_INFO(l_string);
  /// 设置选择
  l_s = MGlobal::setActiveSelectionList(k_select);
  DOODLE_MAYA_CHICK(l_s);
  l_s = MGlobal::executeCommand(d_str{R"(doWrapArgList "7" {"1","0","1", "2","1","1","0","0"};)"});
  DOODLE_MAYA_CHICK(l_s);

  auto l_name = get_node_name(get_transform(in_low));
  l_s         = k_select.clear();
  DOODLE_MAYA_CHICK(l_s);

  l_s = k_select.add(d_str{fmt::format("{}Base*", l_name)});
  DOODLE_MAYA_CHICK(l_s);
  DOODLE_CHICK(k_select.length() > 0, doodle_error{"无法找到包裹生成的网格"});
  MObject l_r{};
  l_s = k_select.getDependNode(0, l_r);
  DOODLE_MAYA_CHICK(l_s);
  return l_r;
}

/**
 * @brief 使用混合变形将解算节点的动态传递给动画节点
 * 并且会将动画节点的皮肤簇设置为无效,并将解算的动态设置为1
 *
 * @param in_sim_node 传入的解算节点
 * @param in_anim_node 传入的动画节点
 *
 */
void transfer_dynamic(const MObject& in_sim_node, const MObject& in_anim_node) {
  MStatus l_s{};

  /// 先将高模的皮肤簇权重重置为0;
  MFnSkinCluster l_fn_skin_cluster{get_skin_custer(in_anim_node), &l_s};
  l_s = l_fn_skin_cluster.setEnvelope(0);
  DOODLE_MAYA_CHICK(l_s);

  DOODLE_LOG_INFO("找到高模皮肤簇 {}， 并将包裹设置为0 ", l_fn_skin_cluster.name(&l_s));
  DOODLE_MAYA_CHICK(l_s);

  ///  获得名称进行格式化命令
  MFnDagNode l_node{in_anim_node, &l_s};
  DOODLE_MAYA_CHICK(l_s);
  std::string l_aim_name = d_str{l_node.name(&l_s)};
  DOODLE_MAYA_CHICK(l_s);

  l_s = l_node.setObject(in_sim_node);
  DOODLE_MAYA_CHICK(l_s);

  std::string l_sim_name = d_str{l_node.name(&l_s)};
  DOODLE_MAYA_CHICK(l_s);
  DOODLE_LOG_INFO("生成包裹命令 blendShape -automatic {} {};", l_sim_name, l_aim_name);
  /// 这个设置包裹
  MStringArray l_blend{};
  l_s = MGlobal::executeCommand(d_str{fmt::format(R"(blendShape -automatic {} {};)", l_sim_name, l_aim_name)}, l_blend);
  DOODLE_MAYA_CHICK(l_s);
  //      /// 开始设置权重
  MGlobal::executeCommand(d_str{fmt::format(R"(setAttr "{}.{}" 1;)", l_blend[0], l_sim_name)});
};

/**
 * @brief
 * @param in_object 要创建解算网格的网格体
 * @return 解算网格创建的 qlClothShape 和他的下一个标准输出端 kMesh 类型
 */
std::tuple<MObject, MObject> qlCreateCloth(const MObject& in_object) {
  MObject l_mesh{};
  MStatus l_s{};
  /// 创建解算网络
  MSelectionList l_selection_list{};
  l_s = l_selection_list.add(in_object);
  DOODLE_MAYA_CHICK(l_s);
  l_s = MGlobal::setActiveSelectionList(l_selection_list);
  DOODLE_MAYA_CHICK(l_s);
  MString l_cloth_shape_name{};
  l_s = MGlobal::executeCommand(d_str{"qlCreateCloth;"}, l_cloth_shape_name);
  DOODLE_MAYA_CHICK(l_s);
  l_s = l_selection_list.clear();
  DOODLE_MAYA_CHICK(l_s);

  l_s = l_selection_list.add(l_cloth_shape_name, true);
  DOODLE_MAYA_CHICK(l_s);

  MObject l_cloth_shape{};
  l_s = l_selection_list.getDependNode(0, l_cloth_shape);
  DOODLE_MAYA_CHICK(l_s);
  auto l_plug = get_plug(l_cloth_shape, "outputMesh");
  for (MItDependencyGraph i{l_plug, MFn::kMesh}; !i.isDone(); i.next()) {
    l_mesh = i.currentItem(&l_s);
    DOODLE_MAYA_CHICK(l_s);
  }
  DOODLE_CHICK(!l_mesh.isNull(), doodle_error{"找不到解算网格的输出端"s});

  return std::make_tuple(l_cloth_shape, l_mesh);
}

/**
 * @brief 创建碰撞体
 * @param in_collider 传入要创建碰撞体的 mobj
 * @return 碰撞体和碰撞偏移物体
 */
std::tuple<MObject, MObject> _add_collider_(const MObject& in_collider) {
  MStatus l_status{};
  /// 创建碰撞体
  auto l_ql_solver = qcloth_shape::get_ql_solver();
  MSelectionList l_list{};
  l_status = l_list.add(l_ql_solver);
  DOODLE_MAYA_CHICK(l_status);
  l_status = l_list.add(in_collider);
  DOODLE_MAYA_CHICK(l_status);
  l_status = MGlobal::setActiveSelectionList(l_list);
  DOODLE_MAYA_CHICK(l_status);
  MString l_collider_name{};
  l_status = MGlobal::executeCommand("qlCreateCollider;", l_collider_name);
  DOODLE_MAYA_CHICK(l_status);

  /// 获取创建出来的碰撞体和解算体
  l_status = l_list.clear();
  l_status = l_list.add(l_collider_name);
  MObject l_collider{};
  l_status = l_list.getDependNode(0, l_collider);

  MObject l_collider_offset{};
  set_attribute(l_collider, "offset", 0.03);

  auto l_out_plug = get_plug(l_collider, "output");
  MPlugArray l_plug_array{};
  l_out_plug.destinations(l_plug_array, &l_status);
  DOODLE_MAYA_CHICK(l_status);
  for (int l_i = 0; l_i < l_plug_array.length(); ++l_i) {
    auto l_node = l_plug_array[l_i].node(&l_status);
    DOODLE_MAYA_CHICK(l_status);
    if (l_node.hasFn(MFn::kMesh)) l_collider_offset = l_node;
  }

  DOODLE_CHICK(
      !l_collider.isNull() && !l_collider_offset.isNull(), doodle_error{"寻找的的解算网格体和偏移网格体不一致"s}
  );
  return std::make_tuple(l_collider, l_collider_offset);
}

/**
 * @brief 检查组节点名称
 * @param in_node 传入的节点
 * @param in_name 节点名称
 * @return 不符合名称的话返回空节点, 否则返回传入节点
 */
MObject chick_group(const MFnDagNode& in_node, const std::string& in_name) {
  MStatus l_s{};
  auto l_name = in_node.name(&l_s);
  DOODLE_MAYA_CHICK(l_s);
  if (l_name.asUTF8() == in_name.c_str()) {
    auto l_obj = in_node.object(&l_s);
    DOODLE_MAYA_CHICK(l_s);
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
MObject make_group(MDagModifier& in_modifier, const std::string& in_name, const MObject& in_parent) {
  MStatus l_s{};
  auto l_r = in_modifier.createNode(d_str{"transform"}, in_parent, &l_s);
  DOODLE_MAYA_CHICK(l_s);
  l_s = in_modifier.renameNode(l_r, d_str{in_name});
  DOODLE_MAYA_CHICK(l_s);
  l_s = in_modifier.doIt();
  DOODLE_MAYA_CHICK(l_s);
  return l_r;
}

cloth_group get_cloth_group() {
  MStatus k_s{};
  cloth_group k_r{};
  MFnDagNode k_node{};

  auto k_reg = g_reg();
  if (k_reg->ctx().contains<cloth_group>()) {
    return k_reg->ctx().get<cloth_group>();
  }

  for (MItDag i{MItDag::kDepthFirst, MFn::Type::kTransform, &k_s}; !i.isDone(); i.next()) {
    DOODLE_MAYA_CHICK(k_s);
    k_s = k_node.setObject(i.currentItem());
    DOODLE_MAYA_CHICK(k_s);
    k_r.cfx_grp        = chick_group(k_node, "cfx_grp");
    k_r.solver_grp     = chick_group(k_node, "solver_grp");
    k_r.anim_grp       = chick_group(k_node, "anim_grp");
    k_r.constraint_grp = chick_group(k_node, "constraint_grp");
    k_r.collider_grp   = chick_group(k_node, "collider_grp");
    k_r.deform_grp     = chick_group(k_node, "deform_grp");
    k_r.export_grp     = chick_group(k_node, "export_grp");
    k_r.deformBase_grp = chick_group(k_node, "deformBase_grp");
  }
  MDagModifier k_m{};
  if (k_r.cfx_grp.isNull()) k_r.cfx_grp = make_group(k_m, "cfx_grp", MObject::kNullObj);

  if (k_r.solver_grp.isNull()) k_r.solver_grp = make_group(k_m, "solver_grp", k_r.cfx_grp);

  if (k_r.anim_grp.isNull()) k_r.anim_grp = make_group(k_m, "anim_grp", k_r.cfx_grp);

  if (k_r.constraint_grp.isNull()) k_r.constraint_grp = make_group(k_m, "constraint_grp", k_r.cfx_grp);

  if (k_r.collider_grp.isNull()) k_r.collider_grp = make_group(k_m, "collider_grp", k_r.cfx_grp);

  if (k_r.deform_grp.isNull()) k_r.deform_grp = make_group(k_m, "deform_grp", k_r.cfx_grp);

  if (k_r.export_grp.isNull()) k_r.export_grp = make_group(k_m, "export_grp", k_r.cfx_grp);

  if (k_r.deformBase_grp.isNull()) k_r.deformBase_grp = make_group(k_m, "deformBase_grp", k_r.deform_grp);

  k_reg->ctx().emplace<cloth_group>(k_r);
  return k_r;
}

/**
 * @brief 从传入的实体创建一个绑定节点
 * @param in_handle 传入的一个实体,
 * 必须具备 qcloth_shape_n::maya_mesh, qcloth_shape_n::high_shape_list组件
 * 可选的具备 qcloth_shape_n::collision_shape_list组件
 *
 *
 * @note
 * * 创建一个空的mesh 节点作为绑定动画的输出;（将动画 outMesh 链接到 inMesh ） \n
 * * 从新的的网格体创建布料 \n
 * * 创建一个高模的复制体, 将低模和高模进行包裹变形 \n
 * * 将复制出来的高模物体链接到绑定文件中（这个以后做  中间需要插入一个切换表达式节点用来切换动画和解算） \n
 *
 *  需要读取配置文件中的各个属性, 进行标准的重命名
 */
void create_sim_cloth(const MObject& in_obj, const std::vector<MObject>& in_list) {
  MAnimControl::setMinTime(MTime{950, MTime::uiUnit()});
  auto l_group = get_cloth_group();

  MStatus k_s{};
  MFnDagNode l_node{};
  MDagModifier l_modifier{};

  auto l_node_name = get_node_name(in_obj);
  {
    if (auto l_f = l_node_name.find("_proxy"); l_f == std::string::npos)
      set_node_name(in_obj, fmt::format("{}_proxy", l_node_name));
    else {
      l_node_name = l_node_name.substr(0, l_f);
    }
  }
  /// \brief 主要的输入节点
  auto k_proxy_node_input  = make_low_node(in_obj, l_group.anim_grp, "input");
  /// \brief 主要的输出节点
  auto k_proxy_node_output = make_low_node(in_obj, l_group.deform_grp, "output");
  /// \brief 动画高模
  auto l_high_mesh         = make_high_node(in_list, l_group.export_grp);

  MDagPath l_path{};

  auto [l_ql, l_mesh_out] = qlCreateCloth(k_proxy_node_input);

  {  /// @brief 设置名称
    set_node_name(get_transform(l_ql), fmt::format("{}_cloth", l_node_name));
    /// \brief 获取ql 创建布料时的附带创建出现的网格
    set_node_name(get_transform(l_mesh_out), fmt::format("{}_cloth_proxy", l_node_name));
  }

  auto l_ql_core = qcloth_shape::get_ql_solver();
  set_attribute(l_ql_core, "frameSamples", 6);
  set_attribute(l_ql_core, "cgAccuracy", 9);
  set_attribute(l_ql_core, "selfCollision", true);
  set_attribute(l_ql_core, "sharpFeature", true);
  {  /// 整理层级关系
    auto l_ql_tran       = get_transform(l_ql);
    auto l_mesh_out_tran = get_transform(l_mesh_out);
    add_child(l_group.solver_grp, l_ql_tran);
    add_child(l_ql_tran, l_mesh_out_tran);
  }
  {  /// 将解算的输出网格连接到代理输出中去
    k_s = l_modifier.connect(get_plug(l_mesh_out, "outMesh"), get_plug(k_proxy_node_output, "inMesh"));
    DOODLE_MAYA_CHICK(k_s);
    k_s = l_modifier.doIt();
    DOODLE_MAYA_CHICK(k_s);
  }
  /// \brief 使用低模包裹高模
  auto k_warp = warp_model(k_proxy_node_output, l_high_mesh);
  add_child(l_group.deformBase_grp, k_warp);
  {
    /// 创建解算网络的输出 这个可以用融合变形(其中先选择主动变形物体, 再选择被变形物体)
    DOODLE_CHICK(l_high_mesh.size() == in_list.size(), doodle_error{"节点数量不一致"s});
    for (int l_i = 0; l_i < l_high_mesh.size(); ++l_i) {
      transfer_dynamic(l_high_mesh[l_i], in_list[l_i]);
    }
  }
}

void add_collider(const std::vector<MObject>& in_coll_list) {
  auto l_group   = get_cloth_group();
  auto l_ql      = qcloth_shape::get_ql_solver();
  auto l_ql_tran = get_transform(l_ql);

  /// \brief 鉴于有些文件会锁定默认着色组, 我们需要进行解锁
  auto k_s       = MGlobal::executeCommand(d_str{R"(lockNode -l false -lu false ":initialShadingGroup";)"});
  DOODLE_MAYA_CHICK(k_s);
  add_child(l_group.cfx_grp, l_ql_tran);

  for (auto& l_item : in_coll_list) {
    auto [l_col, l_col_off] = _add_collider_(l_item);
    auto l_col_tran         = get_transform(l_col);
    auto l_col_off_tran     = get_transform(l_col_off);
    add_child(l_group.collider_grp, l_col_tran);
    add_child(l_group.collider_grp, l_col_off_tran);
  }
}

void sort_group() {
  auto l_group = get_cloth_group();
  MStatus k_s{};
  auto l_ql = get_transform(qcloth_shape::get_ql_solver());
  {  /// \brief 开始排序
    MFnDagNode l_p{};
    k_s = l_p.setObject(l_group.cfx_grp);
    DOODLE_MAYA_CHICK(k_s);

    /// \brief 必须先取消掉父物体
    k_s = l_p.removeChild(l_ql);
    DOODLE_MAYA_CHICK(k_s);
    k_s = l_p.removeChild(l_group.anim_grp);
    DOODLE_MAYA_CHICK(k_s);
    k_s = l_p.removeChild(l_group.solver_grp);
    DOODLE_MAYA_CHICK(k_s);
    k_s = l_p.removeChild(l_group.constraint_grp);
    DOODLE_MAYA_CHICK(k_s);
    k_s = l_p.removeChild(l_group.deform_grp);
    DOODLE_MAYA_CHICK(k_s);
    k_s = l_p.removeChild(l_group.export_grp);
    DOODLE_MAYA_CHICK(k_s);

    /**
     * @brief 排序组
     * - cfx_grp 顺序
     *     - qlsolver
     *     - anim_grp
     *     - solver_grp
     *         - xxx_cloth
     *             - xxx_cloth_proxy
     *     - constraint_grp
     *     - collider_grp
     *     - deform_grp  (包裹的模型)
     *         - xxx_output
     *         - deformBase_grp
     *             - 包裹的base节点
     *     - export_grp
     */

    k_s = l_p.addChild(l_ql, 0);
    DOODLE_MAYA_CHICK(k_s);
    k_s = l_p.addChild(l_group.anim_grp, 1);
    DOODLE_MAYA_CHICK(k_s);
    k_s = l_p.addChild(l_group.solver_grp, 2);
    DOODLE_MAYA_CHICK(k_s);
    k_s = l_p.addChild(l_group.constraint_grp, 3);
    DOODLE_MAYA_CHICK(k_s);
    k_s = l_p.addChild(l_group.deform_grp, 4);
    DOODLE_MAYA_CHICK(k_s);
    k_s = l_p.addChild(l_group.export_grp, 5);
    DOODLE_MAYA_CHICK(k_s);
  }
}

/**
 * @brief 重置传入动画节点(动画[绑定]网格体或者变换节点)链接的皮肤簇属性
 * @param in_anim_node 动画[绑定]网格体或者变换节点
 */
void rest_skin_custer_attr(const MObject& in_anim_node) {
  MStatus l_s{};
  /// 先将高模的皮肤簇权重重置为1;
  MFnSkinCluster l_fn_skin_cluster{get_skin_custer(in_anim_node), &l_s};
  l_s = l_fn_skin_cluster.setEnvelope(1);
  DOODLE_MAYA_CHICK(l_s);
}

/**
 * @brief 重置maya高模皮肤簇节点为权重为1
 * @param in_handle 传入的句柄 需要具备 qcloth_shape_n::shape_list 组件
 */
void reset_create_node_attribute(const std::vector<MObject>& in_list) {
  /// \brief 动画高模
  for (int l_i = 0; l_i < in_list.size(); ++l_i) {
    rest_skin_custer_attr(in_list[l_i]);
  }
}

}  // namespace create_qcloth_assets_ns

class create_qcloth_assets::impl {
 public:
  std::vector<create_qcloth_assets::create_arg> cloth_list{};
  std::vector<MObject> coll_list{};

  std::vector<MObject> create_nodes{};
};

create_qcloth_assets::create_qcloth_assets() : p_i(std::make_unique<impl>()) {}
void create_qcloth_assets::parse_arg(const MArgList& in_arg) {
  DOODLE_LOG_INFO(in_arg);
  MStatus l_s{};
  MArgDatabase const l_arg{syntax(), in_arg, &l_s};

  if (l_arg.isFlagSet(create_qcloth_assets_ns::cloth, &l_s)) {
    maya_chick(l_s);
    auto l_num = l_arg.numberOfFlagUses(create_qcloth_assets_ns::cloth);
    for (auto i = 0; i < l_num; ++i) {
      MSelectionList l_list{};
      MArgList l_arg_list{};
      l_s = l_arg.getFlagArgumentList(create_qcloth_assets_ns::cloth, i, l_arg_list);
      maya_chick(l_s);

      MString l_names = l_arg_list.asString(0, &l_s);
      MStringArray l_high_names{};
      maya_chick(l_names.split(';', l_high_names));
      if (l_high_names.length() < 2) {
        throw_exception(doodle_error{"传入的参数不足"s});
      }
      create_arg l_arg{};

      maya_chick(l_list.add(l_high_names[0]));
      maya_chick(l_list.getDependNode(0, l_arg.low_obj_));

      for (int l_i = 1; l_i < l_high_names.length(); ++l_i) {
        maya_chick(l_list.add(l_high_names[l_i]));
        MObject l_obj{};
        maya_chick(l_list.getDependNode(l_i, l_obj));
        l_arg.high_obj_list_.emplace_back(l_obj);
      }

      p_i->cloth_list.emplace_back(l_arg);
    }
  }

  if (l_arg.isFlagSet(create_qcloth_assets_ns::collision, &l_s)) {
    maya_chick(l_s);
    auto l_num = l_arg.numberOfFlagUses(create_qcloth_assets_ns::collision);
    MSelectionList l_list{};
    for (auto i = 0; i < l_num; ++i) {
      MArgList l_arg_list{};
      l_s = l_arg.getFlagArgumentList(create_qcloth_assets_ns::collision, i, l_arg_list);
      maya_chick(l_s);
      MString l_names = l_arg_list.asString(0, &l_s);
      maya_chick(l_s);
      MObject l_tmp_obj{};
      maya_chick(l_list.add(l_names));
      maya_chick(l_list.getDependNode(0, l_tmp_obj));
      p_i->coll_list.emplace_back(l_tmp_obj);
    }
  }

  if (p_i->cloth_list.empty()) throw_exception(doodle_error{"没有传入的布料"s});
}
MStatus create_qcloth_assets::doIt(const MArgList& in_arg) {
  parse_arg(in_arg);
  // return MStatus::kSuccess;
  return redoIt();
}
MStatus create_qcloth_assets::undoIt() {
  // 删除所有的创建成功的层级
  delete_node();
  // 更新所有的属性
  reset_properties();
  if (g_reg()->ctx().contains<qcloth_shape::cloth_group>()) {
    g_reg()->ctx().erase<qcloth_shape::cloth_group>();
  }
  return MStatus::kSuccess;
}
MStatus create_qcloth_assets::redoIt() {
  try {
    for (auto& l_h : p_i->cloth_list) {
      create_qcloth_assets_ns::create_sim_cloth(l_h.low_obj_, l_h.high_obj_list_);
    }
    if (!p_i->cloth_list.empty()) create_qcloth_assets_ns::add_collider(p_i->coll_list);

    create_qcloth_assets_ns::sort_group();
  } catch (const std::runtime_error& in_err) {
    delete_node();
    return {MStatus::kFailure};
  }

  return MStatus::kSuccess;
}

std::vector<MObject> create_qcloth_assets::get_all_node() {
  std::vector<MObject> l_r{};
  MStatus l_s{};
  for (MItDag l_it{}; !l_it.isDone(); l_it.next()) {
    l_r.emplace_back(l_it.currentItem(&l_s));
    DOODLE_MAYA_CHICK(l_s);
  }
  return l_r;
}

bool create_qcloth_assets::isUndoable() const { return true; }
void create_qcloth_assets::delete_node() {
  MGlobal::deleteNode(g_reg()->ctx().get<qcloth_shape::cloth_group>().cfx_grp);
}

void create_qcloth_assets::reset_properties() {
  for (auto& l_h : p_i->cloth_list) {
    create_qcloth_assets_ns::reset_create_node_attribute(l_h.high_obj_list_);
  }
}

create_qcloth_assets::~create_qcloth_assets() = default;

}  // namespace doodle::maya_plug
