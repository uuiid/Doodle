#include "dna_calib_node.h"

#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MStatus.h>

namespace doodle::maya_plug {
void* dna_calib_node::creator() { return new dna_calib_node{}; }
MStatus dna_calib_node::initialize() {
  MStatus l_status{};

  {  // DNA文件路径
    MFnTypedAttribute l_typed_attr{};
    dna_file_path = l_typed_attr.create("dna_file_path", "dna_p", MFnData::kString, MObject::kNullObj, &l_status);
    CHECK_MSTATUS_AND_RETURN_IT(l_status);
    l_status = l_typed_attr.setStorable(true);
    CHECK_MSTATUS_AND_RETURN_IT(l_status);
    l_status = l_typed_attr.setWritable(true);
    CHECK_MSTATUS_AND_RETURN_IT(l_status);

    l_status = addAttribute(dna_file_path);
    CHECK_MSTATUS_AND_RETURN_IT(l_status);
  }
  return l_status;
}
}  // namespace doodle::maya_plug