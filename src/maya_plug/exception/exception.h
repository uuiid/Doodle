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

class maya_Failure : public maya_error {
 public:
  explicit maya_Failure(const std::string& in_str) : maya_error(in_str){};
};
class maya_InsufficientMemory : public maya_error {
 public:
  explicit maya_InsufficientMemory(const std::string& in_str) : maya_error(in_str){};
};
class maya_InvalidParameter : public maya_error {
 public:
  explicit maya_InvalidParameter(const std::string& in_str) : maya_error(in_str){};
};
class maya_LicenseFailure : public maya_error {
 public:
  explicit maya_LicenseFailure(const std::string& in_str) : maya_error(in_str){};
};
class maya_UnknownParameter : public maya_error {
 public:
  explicit maya_UnknownParameter(const std::string& in_str) : maya_error(in_str){};
};
class maya_NotImplemented : public maya_error {
 public:
  explicit maya_NotImplemented(const std::string& in_str) : maya_error(in_str){};
};
class maya_NotFound : public maya_error {
 public:
  explicit maya_NotFound(const std::string& in_str) : maya_error(in_str){};
};
class maya_EndOfFile : public maya_error {
 public:
  explicit maya_EndOfFile(const std::string& in_str) : maya_error(in_str){};
};

}  // namespace doodle::maya_plug
