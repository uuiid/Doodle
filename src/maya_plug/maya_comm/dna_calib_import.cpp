#include "dna_calib_import.h"

#include "maya_plug_fwd.h"
#include <maya_plug/data/maya_conv_str.h>
#include <maya_plug/data/maya_tool.h>
#include <maya_plug/node/dna_calib_node.h>
#include <maya_plug/node/dna_calib_node_impl.h>

#include <arrayview/ArrayView.h>
#include <dnacalib/DNACalib.h>
#include <fmt/format.h>
#include <maya/MAngle.h>
#include <maya/MArgDatabase.h>
#include <maya/MDagModifier.h>
#include <maya/MDagPath.h>
#include <maya/MEulerRotation.h>
#include <maya/MFloatArray.h>
#include <maya/MFn.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnIkJoint.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSet.h>
#include <maya/MFnTransform.h>
#include <maya/MGlobal.h>
#include <maya/MIntArray.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPointArray.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>
#include <maya/MSyntax.h>
#include <maya/MTypes.h>
#include <maya/MVector.h>
#include <numeric>
#include <pma/ScopedPtr.h>
#include <string>
#include <utility>
#include <vector>

namespace fmt {
// fmt dna StringView
template <>
struct formatter<dna::StringView> : formatter<string_view> {
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.end(); }

  template <typename FormatContext>
  auto format(const dna::StringView& sv, FormatContext& ctx) const -> decltype(ctx.out()) {
    return formatter<string_view>::format(std::string_view{sv.data(), sv.size()}, ctx);
  }
};

}  // namespace fmt

namespace doodle::maya_plug {
MSyntax dna_calib_import_syntax() {
  MSyntax l_syntax{};
  CHECK_MSTATUS(l_syntax.setObjectType(MSyntax::kSelectionList, 1, 1));
  l_syntax.useSelectionAsDefault(true);
  return l_syntax;
}

class dna_calib_import::impl {
 public:
  FSys::path file_path;
  MObject dna_node_obj{};
  dna_calib_node* dna_node_data{};

  MDagModifier dag_modifier_{};

  MObject head_grp_obj_{};
  MObject geometry_grp_obj_{};
  MObject rig_grp_obj_{};

  struct lod_group_info {
    MObject lod_grp_obj_;
    dnac::ConstArrayView<std::uint16_t> mesh_indices_;
  };
  struct mesh_info {
    MObject mesh_obj_;
    std::string name_;
  };
  std::vector<lod_group_info> lod_grp_objs_{};
  std::vector<mesh_info> imported_meshes_{};

  dnac::ScopedPtr<dnac::DNACalibDNAReader>& get_dna_reader() { return dna_node_data->impl()->dna_calib_dna_reader_; }
  void conv_units() {
    auto l_translation_unit = get_dna_reader()->getTranslationUnit();
    switch (l_translation_unit) {
      case dna::TranslationUnit::cm:
        display_warning("当前 dna 文件使用厘米单位, Maya 默认使用厘米单位, 无需转换");
        break;
      case dna::TranslationUnit::m:
        display_info("当前 dna 文件使用米单位, 需要转换为厘米单位");
        break;
      default:
        break;
    }
    auto l_rotation_unit = get_dna_reader()->getRotationUnit();
    switch (l_rotation_unit) {
      case dna::RotationUnit::degrees:
        display_warning("当前 dna 文件使用角度单位, Maya 默认使用角度单位, 无需转换");
        break;
      case dna::RotationUnit::radians:
        display_info("当前 dna 文件使用弧度单位, 需要转换为角度单位");
        break;
      default:
        break;
    }
  }

  MStatus import_dna_calib() {
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(dna_node_data->impl()->open_dna_file());

    conv_units();
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(connect_gui_controls());
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(create_groups());

    for (auto i = 0; i < get_dna_reader()->getMeshCount(); ++i) {
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(create_mesh_from_dna_mesh(i, get_mesh_lod_group(i)));
    }
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(create_joints());

    return MS::kSuccess;
  }

  // 创建 骨骼
  MStatus create_joints() {
    MStatus l_status{};
    std::vector<MObject> joint_objs{};
    auto l_joint_count = get_dna_reader()->getJointCount();
    joint_objs.reserve(l_joint_count);
    for (auto i = 0; i < l_joint_count; ++i) {
      auto l_joint_name = get_dna_reader()->getJointName(i);
      display_info("创建骨骼: {} {}", i, l_joint_name);
      MObject l_joint_obj = dag_modifier_.createNode("joint", MObject::kNullObj, &l_status);
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(dag_modifier_.renameNode(l_joint_obj, conv::to_ms(std::string{l_joint_name})));
      joint_objs.push_back(l_joint_obj);
    }
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(dag_modifier_.doIt());
    for (auto i = 0; i < l_joint_count; ++i) {
      auto l_parent_index = get_dna_reader()->getJointParentIndex(i);
      if (l_parent_index == i) {
        // 将根骨骼绑定到 rig组下
        DOODLE_CHECK_MSTATUS_AND_RETURN_IT(dag_modifier_.reparentNode(joint_objs[i], head_grp_obj_));
      } else {
        DOODLE_CHECK_MSTATUS_AND_RETURN_IT(
            dag_modifier_.reparentNode(joint_objs[i], joint_objs[static_cast<std::size_t>(l_parent_index)])
        );
      }
    }
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(dag_modifier_.doIt());

    MFnDagNode l_dag_node_fn{};
    MFnIkJoint l_ik_joint_fn{};
    MDagPath l_dag_path{};
    for (auto i = 0; i < l_joint_count; ++i) {
      auto l_parent_index = get_dna_reader()->getJointParentIndex(i);

      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_dag_node_fn.setObject(joint_objs[i]));
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_dag_node_fn.getPath(l_dag_path));
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_ik_joint_fn.setObject(l_dag_path));

      auto l_rotation_value    = get_dna_reader()->getNeutralJointRotation(i);
      auto l_translation_value = get_dna_reader()->getNeutralJointTranslation(i);
      // 需要将分量从角度转为弧度
      // DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_transform_fn.setRotation(
      //     MEulerRotation{
      //         MAngle{l_rotation_value.x, MAngle::kDegrees}.asRadians(),
      //         MAngle{l_rotation_value.y, MAngle::kDegrees}.asRadians(),
      //         MAngle{l_rotation_value.z, MAngle::kDegrees}.asRadians(),
      //     }
      // ));
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_ik_joint_fn.setOrientation(
          MEulerRotation{
              MAngle{l_rotation_value.x, MAngle::kDegrees}.asRadians(),
              MAngle{l_rotation_value.y, MAngle::kDegrees}.asRadians(),
              MAngle{l_rotation_value.z, MAngle::kDegrees}.asRadians(),
          }
      ));
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_ik_joint_fn.setTranslation(
          MVector{l_translation_value.x, l_translation_value.y, l_translation_value.z},
          l_parent_index == i ? MSpace::kWorld : MSpace::kTransform
      ));
    }

    return MS::kSuccess;
  }

  MStatus connect_gui_controls() {
    MStatus l_status{};
    MPlug l_dna_file_path_plug{dna_node_obj, dna_calib_node::gui_control_list};
    auto l_gui_control_count = get_dna_reader()->getGUIControlCount();
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(
        l_dna_file_path_plug.setNumElements(static_cast<unsigned int>(l_gui_control_count))
    );

    for (auto i = 0; i < get_dna_reader()->getGUIControlCount(); ++i) {
      auto l_gui_control_name = get_dna_reader()->getGUIControlName(i);
      auto l_plug             = get_name_obj_attr_for_str(std::string_view{l_gui_control_name});
      if (l_plug.isNull()) {
        display_warning("未找到对应的节点属性: {}", l_gui_control_name);
        continue;
      }
      auto l_target = l_dna_file_path_plug.elementByLogicalIndex(i, &l_status);
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(dag_modifier_.connect(l_plug, l_target));
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    }
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(dag_modifier_.doIt());
    return MS::kSuccess;
  }

  MObject get_name_obj(const char* in_group_name) { return get_name_obj(std::string_view{in_group_name}); }
  MObject get_name_obj(std::string_view in_node_name) {
    MSelectionList l_sel_list{};
    MGlobal::getSelectionListByName(conv::to_ms(std::string{in_node_name}), l_sel_list);
    MDagPath l_obj{};
    if (l_sel_list.length() == 0) return MObject::kNullObj;
    CHECK_MSTATUS_AND_RETURN(l_sel_list.getDagPath(0, l_obj), MObject::kNullObj);
    return l_obj.node();
  }
  MObject get_name_obj_attr(const char* in_attr_name, const MObject& in_node_obj) {
    return get_name_obj_attr(std::string_view{in_attr_name}, in_node_obj);
  }
  MObject get_name_obj_attr(std::string_view in_attr_name, const MObject& in_node_obj) {
    MFnDependencyNode l_dep_node_fn{};
    MStatus l_status = l_dep_node_fn.setObject(in_node_obj);
    CHECK_MSTATUS_AND_RETURN(l_status, MObject::kNullObj);
    MObject l_attr_obj = l_dep_node_fn.attribute(conv::to_ms(std::string{in_attr_name}), &l_status);
    CHECK_MSTATUS_AND_RETURN(l_status, MObject::kNullObj);
    return l_attr_obj;
  }
  MPlug get_name_obj_attr_for_str(std::string_view in_node_attr_name) {
    auto l_pos = in_node_attr_name.find('.');
    if (l_pos == std::string_view::npos) return MPlug{};
    auto l_node_name = in_node_attr_name.substr(0, l_pos);
    auto l_attr_name = in_node_attr_name.substr(l_pos + 1);

    MObject l_obj    = get_name_obj(l_node_name);
    if (l_obj.isNull()) return MPlug{};
    MObject l_attr = get_name_obj_attr(l_attr_name, l_obj);
    if (l_attr.isNull()) return MPlug{};
    return MPlug{l_obj, l_attr};
  }

  MStatus create_groups() {
    constexpr auto g_top_level_group{"head_grp"};
    constexpr auto g_geometry_group{"geometry_grp"};
    constexpr auto g_rig_group{"head_rig_grp"};
    MStatus l_status{};

    MFnTransform l_transform{};
    MObject l_top_level_group = get_name_obj(g_top_level_group);
    if (l_top_level_group.isNull()) {
      //  l_transform.create(MObject::kNullObj, &l_status);
      l_top_level_group = dag_modifier_.createNode("transform", MObject::kNullObj, &l_status);
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(dag_modifier_.renameNode(l_top_level_group, g_top_level_group));
    }
    head_grp_obj_            = l_top_level_group;
    MObject l_geometry_group = get_name_obj(g_geometry_group);
    if (l_geometry_group.isNull()) {
      l_geometry_group = dag_modifier_.createNode("transform", l_top_level_group, &l_status);
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(dag_modifier_.renameNode(l_geometry_group, g_geometry_group));
    }
    geometry_grp_obj_   = l_geometry_group;
    MObject l_rig_group = get_name_obj(g_rig_group);
    if (l_rig_group.isNull()) {
      l_rig_group = dag_modifier_.createNode("transform", l_top_level_group, &l_status);
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(dag_modifier_.renameNode(l_rig_group, g_rig_group));
    }
    rig_grp_obj_ = l_rig_group;
    for (auto i = 0; i < get_dna_reader()->getLODCount(); ++i) {
      auto l_lod_mesh_index = get_dna_reader()->getMeshIndicesForLOD(i);
      auto l_lod_group_name = fmt::format("lod_{}_grp", i);
      MObject l_lod_group   = get_name_obj(l_lod_group_name.data());
      if (l_lod_group.isNull()) {
        l_lod_group = dag_modifier_.createNode("transform", l_geometry_group, &l_status);
        DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
        DOODLE_CHECK_MSTATUS_AND_RETURN_IT(dag_modifier_.renameNode(l_lod_group, l_lod_group_name.data()));
        lod_grp_objs_.push_back({l_lod_group, l_lod_mesh_index});
      }
      if (i != 0) {
        // 需要隐藏其他lod组
        MFnDagNode l_dag_node_fn{};
        DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_dag_node_fn.setObject(l_lod_group));
        MPlug l_vis_plug = l_dag_node_fn.findPlug("visibility", true, &l_status);
        DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
        DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_vis_plug.setValue(false));
      }
    }

    return dag_modifier_.doIt();
  }
  MObject get_mesh_lod_group(std::size_t in_mesh_idx) {
    for (auto&& l_lod_info : lod_grp_objs_) {
      for (auto&& l_mesh_idx : l_lod_info.mesh_indices_) {
        if (l_mesh_idx == in_mesh_idx) return l_lod_info.lod_grp_obj_;
      }
    }
    return MObject::kNullObj;
  }

  MStatus create_mesh_from_dna_mesh(std::size_t in_mesh_idx, const MObject& in_parent = MObject::kNullObj) {
    auto l_name         = get_dna_reader()->getMeshName(in_mesh_idx);

    auto l_vertex_count = get_dna_reader()->getVertexPositionCount(in_mesh_idx);
    if (l_vertex_count == 0) return display_warning("Mesh {} 没有顶点, 跳过创建", l_name), MS::kSuccess;
    auto l_uv_count     = get_dna_reader()->getVertexTextureCoordinateCount(in_mesh_idx);
    auto l_layout_count = get_dna_reader()->getVertexLayoutCount(in_mesh_idx);
    auto l_face_count   = get_dna_reader()->getFaceCount(in_mesh_idx);
    display_info(
        "Mesh {}: 名称: {} 顶点数量: {} uv数量: {} layout数量: {}, 面数量: {}", in_mesh_idx, l_name, l_vertex_count,
        l_uv_count, l_layout_count, l_face_count
    );
    MPointArray l_vertices{};
    l_vertices.setLength(static_cast<unsigned int>(l_vertex_count));
    for (auto j = 0; j < l_vertex_count; ++j) {
      auto l_pos = get_dna_reader()->getVertexPosition(in_mesh_idx, j);
      l_vertices.set(static_cast<unsigned int>(j), l_pos.x, l_pos.y, l_pos.z);
    }
    std::vector<dna::VertexLayout> l_layouts{};
    l_layouts.reserve(static_cast<size_t>(l_layout_count));
    for (auto j = 0; j < l_layout_count; ++j) {
      auto l_layout = get_dna_reader()->getVertexLayout(in_mesh_idx, j);
      l_layouts.push_back(l_layout);
    }

    MIntArray l_face_vertex_counts{};
    MIntArray l_face_vertex_indices{};
    std::vector<dna::ConstArrayView<std::uint32_t>> l_face_layout_indices{};
    l_face_layout_indices.reserve(static_cast<size_t>(l_face_count));
    l_face_vertex_indices.setSizeIncrement(l_face_count);
    l_face_vertex_counts.setLength(static_cast<unsigned int>(l_face_count));
    for (auto j = 0; j < l_face_count; ++j) {
      auto l_indices = get_dna_reader()->getFaceVertexLayoutIndices(in_mesh_idx, j);
      l_face_layout_indices.push_back(l_indices);
      l_face_vertex_counts.set(static_cast<int>(l_indices.size()), static_cast<unsigned int>(j));
      for (auto k = 0; k < l_indices.size(); ++k) {
        l_face_vertex_indices.append(static_cast<int>(l_layouts[l_indices[k]].position));
      }
    }

    std::vector<dna::TextureCoordinate> l_uvs{};
    l_uvs.reserve(static_cast<size_t>(l_uv_count));
    for (auto j = 0; j < l_uv_count; ++j) {
      auto l_uv = get_dna_reader()->getVertexTextureCoordinate(in_mesh_idx, j);
      l_uvs.push_back(l_uv);
    }
    MFloatArray l_u_array{}, l_v_array{};
    MIntArray l_uv_indices{};
    l_u_array.setSizeIncrement(static_cast<unsigned int>(l_uv_count));
    l_v_array.setSizeIncrement(static_cast<unsigned int>(l_uv_count));
    l_uv_indices.setSizeIncrement(static_cast<unsigned int>(l_uv_count));
    std::size_t l_uv_idx_counter = 0;
    for (auto&& l_face : l_face_layout_indices) {
      for (auto&& l_idx : l_face) {
        // l_face_vertex_indices.append(static_cast<int>(l_layouts[l_idx].position));
        auto l_uv_idx = l_layouts[l_idx].textureCoordinate;
        if (l_uv_idx < l_uv_count) {
          l_u_array.append(l_uvs[l_uv_idx].u);
          l_v_array.append(l_uvs[l_uv_idx].v);
          l_uv_indices.append(static_cast<int>(l_uv_idx_counter));
          ++l_uv_idx_counter;
        } else {
          return display_warning(
                     "Mesh {} 面的顶点使用了不存在的uv索引: {}, 总uv数量: {}", l_name, l_uv_idx, l_uv_count
                 ),
                 MS::kFailure;
        }
      }
    }
    MStatus l_status{};
    MObject l_mesh_parent = in_parent;
    if (!l_mesh_parent.isNull()) {
      l_mesh_parent = dag_modifier_.createNode("transform", in_parent, &l_status);
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(dag_modifier_.renameNode(l_mesh_parent, l_name.data()));
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(dag_modifier_.doIt());
    }

    MFnMesh l_fn_mesh{};
    if (auto l_sum = std::reduce(l_face_vertex_counts.begin(), l_face_vertex_counts.end());
        l_sum != static_cast<int>(l_face_vertex_indices.length()))
      return display_error(
                 "面顶点数量与顶点索引数量不匹配, 面顶点数量总和: {}, 顶点索引数量: {}", l_sum,
                 l_face_vertex_indices.length()
             ),
             MS::kFailure;
    // l_u_array, l_v_array,
    l_fn_mesh.create(
        static_cast<unsigned int>(l_vertex_count), static_cast<unsigned int>(l_face_count), l_vertices,
        l_face_vertex_counts, l_face_vertex_indices, l_mesh_parent, &l_status
    );
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_fn_mesh.setUVs(l_u_array, l_v_array));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_fn_mesh.assignUVs(l_face_vertex_counts, l_uv_indices));
    mesh_info l_mesh_info{l_fn_mesh.object(), l_name.data()};
    imported_meshes_.push_back(l_mesh_info);

    // 添加材质
    MFnSet l_fn_set{};
    MSelectionList l_selection;
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_selection.add("initialShadingGroup"));
    if (l_selection.length() == 0) return display_warning("未找到初始着色组"), MS::kSuccess;
    MObject l_init_shading_group_obj{};
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_selection.getDependNode(0, l_init_shading_group_obj));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_fn_set.setObject(l_init_shading_group_obj));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_fn_set.addMember(l_fn_mesh.object()));

    return MS::kSuccess;
  }

  MStatus mesh_blend_shapes(std::size_t in_mesh_idx) {
    auto l_count = get_dna_reader()->getBlendShapeTargetCount(in_mesh_idx);
    for (auto i = 0; i < l_count; ++i) {
      auto l_bl_name_index = get_dna_reader()->getBlendShapeChannelIndex(in_mesh_idx, i);
      auto l_bl_name       = get_dna_reader()->getBlendShapeChannelName(l_bl_name_index);
      std::vector<std::pair<std::size_t, dnac::Vector3>> l_delta_positions{};
      auto l_vertex_indices = get_dna_reader()->getBlendShapeTargetVertexIndices(in_mesh_idx, i);
      auto l_delta_count    = get_dna_reader()->getBlendShapeTargetDeltaCount(in_mesh_idx, i);
      for (auto j = 0; j < l_delta_count; ++j) {
        auto l_vertex_idx = l_vertex_indices[j];
        auto l_pos        = get_dna_reader()->getBlendShapeTargetDelta(in_mesh_idx, i, j);
        l_delta_positions.push_back({l_vertex_idx, l_pos});
      }
    }
    return MS::kSuccess;
  }
};
dna_calib_import::dna_calib_import() : p_i(std::make_unique<impl>()) {}
dna_calib_import::~dna_calib_import() = default;

MStatus dna_calib_import::get_arg(const MArgList& in_arg) {
  MStatus l_status{};
  MArgDatabase const l_arg_data{syntax(), in_arg, &l_status};
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
  MSelectionList l_list{};
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_arg_data.getObjects(l_list));
  for (unsigned int i = 0; i < l_list.length(); ++i) {
    MObject l_node_obj{};
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_list.getDependNode(i, l_node_obj));
    if (!l_node_obj.hasFn(MFn::kDependencyNode)) continue;
    MFnDependencyNode l_fn_node{l_node_obj, &l_status};
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    if (l_fn_node.typeId() != dna_calib_node::doodle_id) continue;
    p_i->dna_node_obj  = l_node_obj;
    p_i->dna_node_data = static_cast<dna_calib_node*>(l_fn_node.userNode());
    break;
  }
  return MS::kSuccess;
}

MStatus dna_calib_import::doIt(const MArgList& in_list) {
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(get_arg(in_list));

  if (p_i->dna_node_obj.isNull() || p_i->dna_node_data == nullptr)
    return display_error("未选择dna_calib_node节点"), MS::kFailure;
  // 读取文件
  return p_i->import_dna_calib();
}

}  // namespace doodle::maya_plug