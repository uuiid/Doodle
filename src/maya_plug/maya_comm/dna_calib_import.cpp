#include "dna_calib_import.h"

#include "maya_plug_fwd.h"
#include <maya_plug/data/dagpath_cmp.h>
#include <maya_plug/data/maya_conv_str.h>
#include <maya_plug/data/maya_tool.h>
#include <maya_plug/fmt/fmt_dag_path.h>
#include <maya_plug/fmt/fmt_warp.h>
#include <maya_plug/node/dna_calib_node.h>
#include <maya_plug/node/dna_calib_node_impl.h>

#include <algorithm>
#include <arrayview/ArrayView.h>
#include <dnacalib/DNACalib.h>
#include <fmt/format.h>
#include <map>
#include <maya/MAngle.h>
#include <maya/MArgDatabase.h>
#include <maya/MDagModifier.h>
#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MEulerRotation.h>
#include <maya/MFloatArray.h>
#include <maya/MFn.h>
#include <maya/MFnBlendShapeDeformer.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnIkJoint.h>
#include <maya/MFnMesh.h>
#include <maya/MFnMeshData.h>
#include <maya/MFnSet.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MFnTransform.h>
#include <maya/MGlobal.h>
#include <maya/MIntArray.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MItGeometry.h>
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
  std::vector<MObject> joint_objs_{};
  struct joint_to_mesh {
    std::vector<MObject> joint_objs_;
    std::vector<std::string> joint_names_;
    std::vector<std::size_t> joint_index_;
  };
  std::vector<joint_to_mesh> joint_to_mesh_links_{};

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
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(create_bind());
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(create_blend_shape());

    return MS::kSuccess;
  }

  // 创建混合变形
  MStatus create_blend_shape() {
    if (imported_meshes_.empty()) return display_warning("没有可创建 blendShape 的网格, 跳过"), MS::kSuccess;
    for (auto&& i : get_dna_reader()->getMeshIndicesForLOD(0)) {
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(create_blend_shape_for_mesh(i));
    }
    return MS::kSuccess;
  }
  MStatus create_blend_shape_for_mesh(std::size_t in_mesh_index) {
    if (in_mesh_index >= imported_meshes_.size()) {
      return display_warning("mesh index {} 超出 imported_meshes_ 范围, 跳过 blendShape", in_mesh_index), MS::kSuccess;
    }
    auto& l_mesh_info = imported_meshes_[in_mesh_index];
    if (l_mesh_info.mesh_obj_.isNull()) {
      return display_warning("mesh {} 对象为空, 跳过 blendShape", l_mesh_info.name_), MS::kSuccess;
    }

    const auto l_target_count = get_dna_reader()->getBlendShapeTargetCount(in_mesh_index);
    if (l_target_count == 0) return MS::kSuccess;

    MStatus l_status{};

    // base mesh
    MFnMesh l_base_mesh_fn{};
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_base_mesh_fn.setObject(l_mesh_info.mesh_obj_));

    // // create blendShape deformer
    // MFnBlendShapeDeformer l_blend_fn{};
    // // Maya API: create a blendShape for the given base object
    // MObject l_blend_obj = l_blend_fn.create(l_mesh_info.mesh_obj_, MFnBlendShapeDeformer::kLocalOrigin, &l_status);
    // DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    // DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_blend_fn.setObject(l_blend_obj));

    // {
    //   DOODLE_CHECK_MSTATUS_AND_RETURN_IT(
    //       dag_modifier_.renameNode(l_blend_obj, conv::to_ms(fmt::format("{}_blendShape", l_mesh_info.name_)))
    //   );
    //   DOODLE_CHECK_MSTATUS_AND_RETURN_IT(dag_modifier_.doIt());
    // }

    // read base points once
    MPointArray l_base_points{};
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_base_mesh_fn.getPoints(l_base_points, MSpace::kObject));

    // batch add targets via one MEL command for stability and performance
    struct blend_target_info {
      int weight_index{};
      std::string channel_name;
      MObject target_mesh_obj;
      MDagPath target_path;
    };
    std::vector<blend_target_info> l_targets_to_add{};
    l_targets_to_add.reserve(static_cast<std::size_t>(l_target_count));

    MDagPath l_base_path{};
    {
      MFnDagNode l_dag_fn{l_mesh_info.mesh_obj_, &l_status};
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_dag_fn.getPath(l_base_path));
    }

    // MFnDependencyNode l_blend_dep_fn{l_blend_obj, &l_status};
    // DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);

    std::int32_t l_next_weight_index{};
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);

    // for each target, build a target mesh in meshData and add it
    for (std::uint16_t i = 0; i < l_target_count; ++i) {
      const auto l_channel_index  = get_dna_reader()->getBlendShapeChannelIndex(in_mesh_index, i);
      const auto l_channel_name   = get_dna_reader()->getBlendShapeChannelName(l_channel_index);

      auto l_points               = l_base_points;
      const auto l_vertex_indices = get_dna_reader()->getBlendShapeTargetVertexIndices(in_mesh_index, i);
      const auto l_delta_count    = get_dna_reader()->getBlendShapeTargetDeltaCount(in_mesh_index, i);

      if (l_vertex_indices.size() != l_delta_count) {
        display_warning(
            "mesh {} blendShape {} 的 vertexIndices({}) 与 deltaCount({}) 不一致", l_mesh_info.name_,
            std::string_view{l_channel_name}, l_vertex_indices.size(), l_delta_count
        );
      }

      const auto l_apply_count = std::min<std::size_t>(l_vertex_indices.size(), l_delta_count);
      for (std::size_t j = 0; j < l_apply_count; ++j) {
        const auto l_vertex_idx = static_cast<unsigned int>(l_vertex_indices[j]);
        if (l_vertex_idx >= l_points.length()) {
          display_warning(
              "mesh {} blendShape {} 的顶点索引 {} 超出范围(0..{}), 跳过", l_mesh_info.name_,
              std::string_view{l_channel_name}, l_vertex_idx, l_points.length() == 0 ? 0 : (l_points.length() - 1)
          );
          continue;
        }
        const auto l_delta = get_dna_reader()->getBlendShapeTargetDelta(in_mesh_index, i, j);
        l_points[l_vertex_idx].x += l_delta.x;
        l_points[l_vertex_idx].y += l_delta.y;
        l_points[l_vertex_idx].z += l_delta.z;
      }

      auto l_bs_tran = dag_modifier_.createNode("transform", l_mesh_info.mesh_obj_, &l_status);
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(dag_modifier_.doIt());

      // 这里不要直接使用 MFnBlendShapeDeformer::addTarget(...)。
      // 在不同 Maya 版本/对象类型组合下，该 API 很容易报 (kInvalidParameter): 对象与此方法不兼容。
      // 使用等价的 MEL 命令更稳定：blendShape -e -t base index target weight blendShapeNode
      MFnMesh l_target_mesh_fn{};
      MObject l_target_mesh_obj = l_target_mesh_fn.copy(l_mesh_info.mesh_obj_, l_bs_tran, &l_status);
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_target_mesh_fn.setPoints(l_points, MSpace::kObject));
      // 重命名网格体为目标混变名称
      {
        DOODLE_CHECK_MSTATUS_AND_RETURN_IT(dag_modifier_.renameNode(l_target_mesh_obj, l_channel_name.c_str()));
        DOODLE_CHECK_MSTATUS_AND_RETURN_IT(dag_modifier_.doIt());
      }

      // add target (each target uses a unique weight index)
      {
        MDagPath l_target_path{};
        {
          MFnDagNode l_dag_fn{l_target_mesh_obj, &l_status};
          DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
          DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_dag_fn.getPath(l_target_path));
        }
        l_targets_to_add.push_back(
            blend_target_info{l_next_weight_index++, std::string{l_channel_name}, l_target_mesh_obj, l_target_path}
        );
      }
    }

    if (!l_targets_to_add.empty()) {
      std::vector<MDagPath> l_target_paths{};
      for (const auto& t : l_targets_to_add) {
        l_target_paths.push_back(t.target_path);
      }
      auto l_mel = fmt::format(R"(blendShape {} {})", fmt::join(l_target_paths, " "), l_mesh_info.name_);
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(MGlobal::executeCommand(conv::to_ms(l_mel)));

      // alias weight[weightIndex] with channel name
      // for (const auto& t : l_targets_to_add) {
      //   const auto l_alias_mel = fmt::format(
      //       R"(catchQuiet(`aliasAttr -rm \"{}\"`); aliasAttr -add \"{}\" \"{}.weight[{}]\";)", t.channel_name,
      //       t.channel_name, l_blend_dep_fn.name().asChar(), t.weight_index
      //   );
      //   DOODLE_CHECK_MSTATUS_AND_RETURN_IT(MGlobal::executeCommand(conv::to_ms(l_alias_mel)));
      // }

      // delete all temporary target meshes
      for (const auto& t : l_targets_to_add) {
        DOODLE_CHECK_MSTATUS_AND_RETURN_IT(dag_modifier_.deleteNode(t.target_mesh_obj, true));
      }
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(dag_modifier_.doIt());
    }

    return MS::kSuccess;
  }

  // 创建绑定
  MStatus create_bind() {
    if (imported_meshes_.empty()) return display_warning("没有可绑定的网格, 跳过绑定"), MS::kSuccess;
    if (joint_objs_.empty()) return display_warning("没有可绑定的骨骼, 跳过绑定"), MS::kSuccess;

    get_mesh_for_joint(0);
    for (auto&& i : get_dna_reader()->getMeshIndicesForLOD(0)) {
      auto l_bind_mel = fmt::format(
          "skinCluster -toSelectedBones -bindMethod 0 -skinMethod 0 -normalizeWeights 2 {} {}",
          imported_meshes_[i].name_, fmt::join(joint_to_mesh_links_[i].joint_names_, " ")
      );
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(MGlobal::executeCommand(conv::to_ms(l_bind_mel)));
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(set_skin_cluster_weights(i));
    }

    return MS::kSuccess;
  }

  void get_mesh_for_joint(std::size_t in_lod_index) {
    joint_to_mesh_links_.resize(get_dna_reader()->getMeshCount());
    for (auto&& l_mesh_index : get_dna_reader()->getMeshIndicesForLOD(0)) {
      auto l_vertex_count = get_dna_reader()->getVertexPositionCount(l_mesh_index);
      for (auto i = 0; i < l_vertex_count; ++i) {
        auto l_joint_indices = get_dna_reader()->getSkinWeightsJointIndices(l_mesh_index, i);
        for (auto j = 0; j < l_joint_indices.size(); ++j) {
          auto l_dna_joint_index = l_joint_indices[j];
          if (l_dna_joint_index >= joint_objs_.size()) {
            display_warning("顶点 {} 的骨骼索引 {} 超出范围, 跳过该权重", i, l_dna_joint_index);
            continue;
          }
          auto l_joint_obj = joint_objs_[l_dna_joint_index];
          if (!l_joint_obj.isNull()) {
            joint_to_mesh_links_[l_mesh_index].joint_objs_.push_back(l_joint_obj);
            joint_to_mesh_links_[l_mesh_index].joint_names_.push_back(get_node_full_name(l_joint_obj));
            joint_to_mesh_links_[l_mesh_index].joint_index_.push_back(l_dna_joint_index);
          } else {
            display_warning("顶点 {} 的骨骼索引 {} 对应的骨骼对象为空, 跳过该权重", i, l_dna_joint_index);
          }
        }
      }
    }
  }

  MStatus set_skin_cluster_weights(std::size_t in_mesh_index) {
    MObject l_skin_cluster_obj{};
    MStatus l_status{};
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(get_skin_cluster(in_mesh_index, l_skin_cluster_obj));
    MFnSkinCluster l_skin_node_fn{};
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_skin_node_fn.setObject(l_skin_cluster_obj));
    MDagPathArray l_joint_paths{};
    const auto l_joint_cout = l_skin_node_fn.influenceObjects(l_joint_paths, &l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);

    MDagPath l_mesh_dag_path{};
    l_mesh_dag_path     = get_dag_path(imported_meshes_[in_mesh_index].mesh_obj_);
    auto l_vertex_count = get_dna_reader()->getVertexPositionCount(in_mesh_index);

    std::map<std::size_t, std::size_t> l_dna_joint_index_to_maya_influence_index{};
    MIntArray l_joint_indices_array{};
    std::map<std::size_t, std::size_t> l_maya_logical_influence_index_to_column{};
    {
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_joint_indices_array.setLength(l_joint_cout));
      std::map<MDagPath, std::size_t, details::cmp_dag> l_dag_path_to_maya_influence_index{};
      for (auto i = 0; i < l_joint_cout; ++i) {
        const auto l_logical_index = l_skin_node_fn.indexForInfluenceObject(l_joint_paths[i], &l_status);
        DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
        l_dag_path_to_maya_influence_index.emplace(l_joint_paths[i], l_logical_index);

        DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_joint_indices_array.set(static_cast<int>(l_logical_index), i));
        l_maya_logical_influence_index_to_column.emplace(
            static_cast<std::size_t>(l_logical_index), static_cast<std::size_t>(i)
        );
      }
      for (auto&& l_joint_index : joint_to_mesh_links_[in_mesh_index].joint_index_) {
        auto l_dag_path = get_dag_path(joint_objs_[l_joint_index]);
        if (l_dag_path_to_maya_influence_index.contains(l_dag_path)) {
          l_dna_joint_index_to_maya_influence_index.emplace(
              l_joint_index, l_dag_path_to_maya_influence_index[l_dag_path]
          );
        } else {
          display_warning(
              "未找到骨骼 {} 的 Maya 影响对象, 可能导致权重丢失", get_node_name(joint_objs_[l_joint_index])
          );
        }
      }
    }
    {
      for (auto i = 0; i < l_joint_cout; ++i) {
      }
    }

    // display_info(
    //     "网格 {} Maya 影响对象数量 {}, 匹配上的骨骼 {}", imported_meshes_[in_mesh_index].name_, l_joint_cout,
    //     l_dna_joint_index_to_maya_influence_index
    // );

    MFnSingleIndexedComponent l_skin_component_fn{};
    l_skin_component_fn.create(MFn::kMeshVertComponent, &l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    {
      MIntArray l_vertex_indices_array{};
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_vertex_indices_array.setLength(l_vertex_count));
      for (auto i = 0; i < l_vertex_count; ++i) {
        DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_vertex_indices_array.set(i, i));
      }
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_skin_component_fn.addElements(l_vertex_indices_array));
    }

    // 清空所有权重
    MDoubleArray l_weights_array{l_joint_cout, 0.0};
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_skin_node_fn.setWeights(
        l_mesh_dag_path, l_skin_component_fn.object(), l_joint_indices_array, l_weights_array, false
    ));
    l_weights_array = MDoubleArray{l_joint_cout * l_vertex_count, 0.0};
    //
    for (auto i = 0; i < l_vertex_count; ++i) {
      auto l_joint_indices = get_dna_reader()->getSkinWeightsJointIndices(in_mesh_index, i);
      auto l_weights       = get_dna_reader()->getSkinWeightsValues(in_mesh_index, i);
      for (auto j = 0; j < l_joint_indices.size(); ++j) {
        auto l_dna_joint_index = l_joint_indices[j];
        if (!l_dna_joint_index_to_maya_influence_index.contains(l_dna_joint_index)) {
          display_warning("未找到骨骼索引 {} 对应的 Maya 影响对象, 跳过该权重", l_dna_joint_index);
          continue;
        }
        const auto l_maya_logical_influence_index = l_dna_joint_index_to_maya_influence_index[l_dna_joint_index];
        if (!l_maya_logical_influence_index_to_column.contains(l_maya_logical_influence_index)) {
          display_warning(
              "未找到 Maya influence logicalIndex {} 对应的列索引, 跳过该权重", l_maya_logical_influence_index
          );
          continue;
        }
        const auto l_column_index = l_maya_logical_influence_index_to_column[l_maya_logical_influence_index];
        l_weights_array[i * l_joint_cout + l_column_index] = l_weights[j];
      }
    }
    // display_info("影响列表 {}", l_joint_indices_array);
    // for (auto i = 0; i < l_weights_array.length(); i += l_joint_cout) {
    //   display_info("顶点 {} 权重 {}", i / l_joint_cout, MDoubleArray(&l_weights_array[i], l_joint_cout));
    //   break;
    // }
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_skin_node_fn.setWeights(
        l_mesh_dag_path, l_skin_component_fn.object(), l_joint_indices_array, l_weights_array, false
    ));

    return MS::kSuccess;
  }

  MStatus get_skin_cluster(std::size_t in_mesh_index, MObject& out_skin_cluster_obj) {
    MStatus l_status{};
    MObject l_skin_cluster_obj{};
    MObject in_mesh_obj = imported_meshes_[in_mesh_index].mesh_obj_;
    for (MItDependencyGraph i{in_mesh_obj, MFn::kSkinClusterFilter, MItDependencyGraph::Direction::kUpstream};
         !i.isDone(); i.next()) {
      l_skin_cluster_obj = i.currentItem(&l_status);
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
      if (!l_skin_cluster_obj.isNull()) {
        out_skin_cluster_obj = l_skin_cluster_obj;
        return MS::kSuccess;
      }
    }
    return display_warning("未找到网格 {} 的皮肤簇", imported_meshes_[in_mesh_index].name_), MS::kSuccess;
  }

  // 创建 骨骼
  MStatus create_joints() {
    MStatus l_status{};
    auto l_joint_count = get_dna_reader()->getJointCount();
    joint_objs_.clear();
    joint_objs_.reserve(l_joint_count);
    for (auto i = 0; i < l_joint_count; ++i) {
      auto l_joint_name   = get_dna_reader()->getJointName(i);
      // display_info("创建骨骼: {} {}", i, l_joint_name);
      MObject l_joint_obj = dag_modifier_.createNode("joint", MObject::kNullObj, &l_status);
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(dag_modifier_.renameNode(l_joint_obj, conv::to_ms(std::string{l_joint_name})));
      joint_objs_.push_back(l_joint_obj);
    }
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(dag_modifier_.doIt());
    for (auto i = 0; i < l_joint_count; ++i) {
      auto l_parent_index = get_dna_reader()->getJointParentIndex(i);
      if (l_parent_index == i) {
        // 将根骨骼绑定到 rig组下
        DOODLE_CHECK_MSTATUS_AND_RETURN_IT(dag_modifier_.reparentNode(joint_objs_[i], head_grp_obj_));
      } else {
        DOODLE_CHECK_MSTATUS_AND_RETURN_IT(
            dag_modifier_.reparentNode(joint_objs_[i], joint_objs_[static_cast<std::size_t>(l_parent_index)])
        );
      }
    }
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(dag_modifier_.doIt());

    MFnDagNode l_dag_node_fn{};
    MFnIkJoint l_ik_joint_fn{};
    MDagPath l_dag_path{};
    for (auto i = 0; i < l_joint_count; ++i) {
      auto l_parent_index = get_dna_reader()->getJointParentIndex(i);

      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_dag_node_fn.setObject(joint_objs_[i]));
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
    // display_info(
    //     "Mesh {}: 名称: {} 顶点数量: {} uv数量: {} layout数量: {}, 面数量: {}", in_mesh_idx, l_name, l_vertex_count,
    //     l_uv_count, l_layout_count, l_face_count
    // );
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
    // 合并 uv
    MGlobal::executeCommand(
        conv::to_ms(
            fmt::format("polyMergeUV -distance 0.01 -constructionHistory false {}", l_fn_mesh.fullPathName().asChar())
        )
    );

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