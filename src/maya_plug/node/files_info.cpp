//
// Created by TD on 2024/1/11.
//

#include "files_info.h"

#include <maya/MTypeId.h>
namespace doodle::maya_plug {
MTypeId doodle_file_info::doodle_id = MTypeId{0x00000001};

void* doodle_file_info::creator() { return new doodle_file_info{}; }
MStatus doodle_file_info::initialize() { return MStatus::kSuccess; }

}  // namespace doodle::maya_plug