//
// Created by TD on 2021/11/22.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/exception/exception.h>
#include <maya/MGlobal.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
namespace doodle::maya_plug {
class play_blast;
using play_blast_ptr = std::shared_ptr<play_blast>;

class d_str {
 public:
  string p_u8_str{};
  template <class MStr, std::enable_if<std::is_same_v<MStr, MString>, bool> = true>
  explicit d_str(const MStr& in)
      : p_u8_str(in.asUTF8()){};

  d_str(const string& in_u8_str)
      : p_u8_str(in_u8_str) {
  }

  inline operator MString() const {
    MString k_r{};
    k_r.setUTF8(p_u8_str.c_str());
  }
  inline operator string() const {
    return p_u8_str;
  }
  inline string str() const {
    return p_u8_str;
  }
};

// inline void chick_maya_status(const MStatus& in_status) {
//   if (in_status != MStatus::MStatusCode::kSuccess) {
//     const MString& l_string = in_status.errorString();
//     MGlobal::displayError(l_string);
//     DOODLE_LOG_ERROR(l_string.asUTF8());
//     throw doodle_error{l_string.asUTF8()};
//   }
// };

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
