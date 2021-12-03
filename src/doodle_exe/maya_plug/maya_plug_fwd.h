//
// Created by TD on 2021/11/22.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/exception/exception.h>
#include <maya/MGlobal.h>
#include <maya/MStatus.h>
namespace doodle::maya_plug {
class play_blast;
using play_blast_ptr = std::shared_ptr<play_blast>;

//inline void chick_maya_status(const MStatus& in_status) {
//  if (in_status != MStatus::MStatusCode::kSuccess) {
//    const MString& l_string = in_status.errorString();
//    MGlobal::displayError(l_string);
//    DOODLE_LOG_ERROR(l_string.asUTF8());
//    throw doodle_error{l_string.asUTF8()};
//  }
//};

#define DOODLE_CHICK(in_status)                          \
  {                                                      \
    if (in_status != MStatus::MStatusCode::kSuccess) {   \
      const MString& l_string = in_status.errorString(); \
      MGlobal::displayError(l_string);                   \
      DOODLE_LOG_ERROR(l_string.asUTF8());               \
      throw doodle_error{l_string.asUTF8()};             \
    }                                                    \
  };

}  // namespace doodle::maya_plug
