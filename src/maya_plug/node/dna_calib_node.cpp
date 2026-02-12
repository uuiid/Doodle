#include "dna_calib_node.h"

#include <maya_plug/node/dna_calib_node_impl.h>

#include <maya/MApiNamespace.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>

namespace doodle::maya_plug {
MObject dna_calib_node::dna_file_path{};
MObject dna_calib_node::gui_control_list{};
MObject dna_calib_node::output_join_transforms{};
MObject dna_calib_node::output_join_rotations{};
MObject dna_calib_node::output_join_scales{};
MObject dna_calib_node::output_blendshape_weights{};

MTypeId dna_calib_node::doodle_id = MTypeId{0x00000002};

dna_calib_node::dna_calib_node() : p_i(std::make_unique<impl_t>()) { p_i->self_ = this; }
dna_calib_node::~dna_calib_node() = default;

void* dna_calib_node::creator() { return new dna_calib_node{}; }
dna_calib_node::impl_t* dna_calib_node::impl() { return p_i.get(); }
MStatus dna_calib_node::initialize() {
  MStatus l_status{};

  {  // DNA文件路径
    MFnTypedAttribute l_typed_attr{};
    dna_file_path = l_typed_attr.create("dna_file_path", "dna_p", MFnData::kString, MObject::kNullObj, &l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_typed_attr.setStorable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_typed_attr.setWritable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_typed_attr.setReadable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(addAttribute(dna_file_path));
  }
  {
    MFnNumericAttribute l_numeric_attr{};
    gui_control_list = l_numeric_attr.create("gui_control_list", "gui_cl", MFnNumericData::kDouble, 0, &l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setStorable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setWritable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setArray(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setReadable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(addAttribute(gui_control_list));
  }
  {
    MFnNumericAttribute l_numeric_attr{};
    output_join_transforms =
        l_numeric_attr.create("output_join_transforms", "out_jt", MFnNumericData::kDouble, 0, &l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setStorable(false));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setWritable(false));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setArray(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setReadable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(addAttribute(output_join_transforms));
  }
  {
    MFnUnitAttribute l_numeric_attr{};
    output_join_rotations =
        l_numeric_attr.create("output_join_rotations", "out_jr", MFnUnitAttribute::kAngle, 0, &l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setStorable(false));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setWritable(false));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setArray(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setReadable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(addAttribute(output_join_rotations));
  }
  {
    MFnNumericAttribute l_numeric_attr{};
    output_join_scales = l_numeric_attr.create("output_join_scales", "out_js", MFnNumericData::kDouble, 0, &l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setStorable(false));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setWritable(false));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setArray(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setReadable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(addAttribute(output_join_scales));
  }
  {
    MFnNumericAttribute l_numeric_attr{};
    output_blendshape_weights =
        l_numeric_attr.create("output_blendshape_weights", "out_bsw", MFnNumericData::kDouble, 0, &l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setStorable(false));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setWritable(false));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setArray(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setReadable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(addAttribute(output_blendshape_weights));
  }
#define DOODLE_ATTRAFF(attr)                                                          \
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(attributeAffects(attr, output_join_transforms)); \
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(attributeAffects(attr, output_join_rotations));  \
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(attributeAffects(attr, output_join_scales));     \
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(attributeAffects(attr, output_blendshape_weights));
  DOODLE_ATTRAFF(gui_control_list);
#undef DOODLE_ATTRAFF
  return l_status;
}
MStatus dna_calib_node::compute(const MPlug& in_plug, MDataBlock& in_data_block) {
  if (in_plug == output_join_transforms || in_plug == output_join_rotations || in_plug == output_join_scales ||
      in_plug == output_blendshape_weights) {
    if (!impl()->rig_instance_ptr_) {
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(impl()->open_dna_file());
      impl()->create_rig_data();
    }
    impl()->compute();
    MPlug l_out_j_t{thisMObject(), output_join_transforms};
    MPlug l_out_j_r{thisMObject(), output_join_rotations};
    MPlug l_out_j_s{thisMObject(), output_join_scales};
    MPlug l_out_bsw{thisMObject(), output_blendshape_weights};

    // 关节输出数据 [tx, ty, tz, rx, ry, rz, sx, sy, sz] * joint_count
    auto l_raw_j = impl()->rig_instance_ptr_->getJointOutputs();
    MStatus l_status{};
    for (auto g = 0; g < impl()->dna_calib_dna_reader_->getJointGroupCount(); ++g) {
      auto l_group_out   = impl()->dna_calib_dna_reader_->getJointGroupOutputIndices(g);
      auto l_joint_index = impl()->dna_calib_dna_reader_->getJointGroupJointIndices(g);
      for (auto j = 0; j < l_joint_index.size(); ++j) {
        auto l_j_index      = l_joint_index[j];
        auto l_base_index   = l_group_out[j] * 9;
        auto l_j_base_index = l_j_index * 3;
        // clang-format off
        l_out_j_t.elementByLogicalIndex(l_j_base_index + 0, &l_status); DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
        l_out_j_t.elementByLogicalIndex(l_j_base_index + 1, &l_status); DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
        l_out_j_t.elementByLogicalIndex(l_j_base_index + 2, &l_status); DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
        l_out_j_r.elementByLogicalIndex(l_j_base_index + 0, &l_status); DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
        l_out_j_r.elementByLogicalIndex(l_j_base_index + 1, &l_status); DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
        l_out_j_r.elementByLogicalIndex(l_j_base_index + 2, &l_status); DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
        l_out_j_s.elementByLogicalIndex(l_j_base_index + 0, &l_status); DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
        l_out_j_s.elementByLogicalIndex(l_j_base_index + 1, &l_status); DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
        l_out_j_s.elementByLogicalIndex(l_j_base_index + 2, &l_status); DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
        // clang-format on
        DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_out_j_t.setValue(l_raw_j[l_base_index + 0]));
        DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_out_j_t.setValue(l_raw_j[l_base_index + 1]));
        DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_out_j_t.setValue(l_raw_j[l_base_index + 2]));
        DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_out_j_r.setValue(l_raw_j[l_base_index + 3]));
        DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_out_j_r.setValue(l_raw_j[l_base_index + 4]));
        DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_out_j_r.setValue(l_raw_j[l_base_index + 5]));
        DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_out_j_s.setValue(l_raw_j[l_base_index + 6]));
        DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_out_j_s.setValue(l_raw_j[l_base_index + 7]));
        DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_out_j_s.setValue(l_raw_j[l_base_index + 8]));
      }
    }
  }
  return MS::kUnknownParameter;
}

}  // namespace doodle::maya_plug