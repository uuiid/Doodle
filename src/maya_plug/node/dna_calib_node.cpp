#include "dna_calib_node.h"

#include <maya_plug/node/dna_calib_node_impl.h>

#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MStatus.h>

namespace doodle::maya_plug {
MObject dna_calib_node::dna_file_path{};
MObject dna_calib_node::gui_control_list{};
MObject dna_calib_node::output_join{};
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
    output_join = l_numeric_attr.create("output_join", "out_jt", MFnNumericData::kDouble, 0, &l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setStorable(false));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setWritable(false));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setArray(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setReadable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(addAttribute(output_join));
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
#define DOODLE_ATTRAFF(attr)                                               \
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(attributeAffects(attr, output_join)); \
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(attributeAffects(attr, output_blendshape_weights));
  DOODLE_ATTRAFF(gui_control_list);
#undef DOODLE_ATTRAFF
  return l_status;
}
MStatus dna_calib_node::compute(const MPlug& in_plug, MDataBlock& in_data_block) { return MS::kUnknownParameter; }

}  // namespace doodle::maya_plug