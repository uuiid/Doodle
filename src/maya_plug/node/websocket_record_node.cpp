//
// Created by TD on 2026/9/4.
//

#include "websocket_record_node.h"

#include <maya/MArrayDataBuilder.h>
#include <maya/MArrayDataHandle.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MStatus.h>

namespace doodle::maya_plug {
MObject websocket_record_node::output_names{};
MObject websocket_record_node::output_values{};

MTypeId websocket_record_node::doodle_id = MTypeId{0x00000004};

websocket_record_node::websocket_record_node()  = default;
websocket_record_node::~websocket_record_node() = default;

void* websocket_record_node::creator() { return new websocket_record_node{}; }

MStatus websocket_record_node::initialize() {
  MStatus l_status{};

  {  // 属性名列表
    MFnTypedAttribute l_typed_attr{};
    output_names = l_typed_attr.create("output_names", "out_n", MFnData::kString, MObject::kNullObj, &l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_typed_attr.setStorable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_typed_attr.setWritable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_typed_attr.setReadable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_typed_attr.setArray(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(addAttribute(output_names));
  }
  {  // 属性值列表
    MFnNumericAttribute l_numeric_attr{};
    output_values = l_numeric_attr.create("output_values", "out_v", MFnNumericData::kDouble, 0, &l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setStorable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setWritable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setReadable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setArray(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(addAttribute(output_values));
  }

  return l_status;
}

MStatus websocket_record_node::compute(const MPlug& in_plug, MDataBlock& in_data_block) {
  return MS::kUnknownParameter;
}

}  // namespace doodle::maya_plug
