#include "dna_calib_node.h"

#include <maya_plug/node/dna_calib_node_impl.h>

#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MStatus.h>

namespace doodle::maya_plug {
MObject dna_calib_node::dna_file_path{};
MObject dna_calib_node::gui_control_list{};
MObject dna_calib_node::output_join_transforms{};
MObject dna_calib_node::output_join_rotations{};
MObject dna_calib_node::output_join_scales{};
MObject dna_calib_node::output_blendshape_weights{};

MTypeId dna_calib_node::doodle_id = MTypeId{0x00000002};

dna_calib_node::dna_calib_node() : p_i(std::make_unique<impl>()) {}
dna_calib_node::~dna_calib_node() = default;

void* dna_calib_node::creator() { return new dna_calib_node{}; }
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
    MFnNumericAttribute l_numeric_attr{};
    output_join_rotations =
        l_numeric_attr.create("output_join_rotations", "out_jr", MFnNumericData::kDouble, 0, &l_status);
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

  return l_status;
}
}  // namespace doodle::maya_plug