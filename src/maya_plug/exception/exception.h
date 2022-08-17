//
// Created by TD on 2021/12/20.
//

#pragma once
#include <doodle_core/exception/exception.h>
#include <doodle_core/logger/logger.h>
#include <maya_plug/fmt/fmt_warp.h>

#include <maya/MStatus.h>
#include <maya/MString.h>

namespace doodle::maya_plug {
class maya_error : public doodle_error {
 public:
  explicit maya_error(const std::string& err)
      : doodle_error(err){};
  explicit maya_error(const MString& in_m_string)
      : doodle_error(in_m_string.asUTF8()){};
};

}  // namespace doodle::maya_plug
