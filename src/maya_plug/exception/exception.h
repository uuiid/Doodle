//
// Created by TD on 2021/12/20.
//

#pragma once
#include <doodle_core/exception/exception.h>
#include <doodle_core/logger/logger.h>

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

class DOODLE_CORE_API maya_category : public bsys::error_category {
 public:
  const char* name() const noexcept;

  std::string message(int ev) const;
  char const* message(int ev, char* buffer, std::size_t len) const noexcept;

  bool failed(int ev) const noexcept;

  bsys::error_condition default_error_condition(int ev) const noexcept;

  static const bsys::error_category& get();
};

}  // namespace doodle::maya_plug
