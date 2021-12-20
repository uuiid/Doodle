//
// Created by TD on 2021/12/20.
//

#pragma once
#include <doodle_lib/exception/exception.h>

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
  explicit maya_Failure(const MString& in_str) : maya_error(in_str){};
};
class maya_InsufficientMemory : public maya_error {
 public:
  explicit maya_InsufficientMemory(const MString& in_str) : maya_error(in_str){};
};
class maya_InvalidParameter : public maya_error {
 public:
  explicit maya_InvalidParameter(const MString& in_str) : maya_error(in_str){};
};
class maya_LicenseFailure : public maya_error {
 public:
  explicit maya_LicenseFailure(const MString& in_str) : maya_error(in_str){};
};
class maya_UnknownParameter : public maya_error {
 public:
  explicit maya_UnknownParameter(const MString& in_str) : maya_error(in_str){};
};
class maya_NotImplemented : public maya_error {
 public:
  explicit maya_NotImplemented(const MString& in_str) : maya_error(in_str){};
};
class maya_NotFound : public maya_error {
 public:
  explicit maya_NotFound(const MString& in_str) : maya_error(in_str){};
};
class maya_EndOfFile : public maya_error {
 public:
  explicit maya_EndOfFile(const MString& in_str) : maya_error(in_str){};
};

inline void throw_maya_exception(const MStatus& in_status, const ::spdlog::source_loc& in_location) {
  switch (in_status.statusCode()) {
    case MStatus::MStatusCode::kSuccess:
      break;
    case MStatus::MStatusCode::kFailure: {
      spdlog::default_logger_raw()->log(in_location, spdlog::level::err, in_status.errorString());
      throw maya_Failure{in_status.errorString()};
    }
    case MStatus::MStatusCode::kInsufficientMemory: {
      spdlog::default_logger_raw()->log(in_location, spdlog::level::err, in_status.errorString());
      throw maya_InsufficientMemory{in_status.errorString()};
    }
    case MStatus::MStatusCode::kInvalidParameter: {
      spdlog::default_logger_raw()->log(in_location, spdlog::level::err, in_status.errorString());
      throw maya_InvalidParameter{in_status.errorString()};
    }
    case MStatus::MStatusCode::kLicenseFailure: {
      spdlog::default_logger_raw()->log(in_location, spdlog::level::err, in_status.errorString());
      throw maya_LicenseFailure{in_status.errorString()};
    }
    case MStatus::MStatusCode::kUnknownParameter: {
      spdlog::default_logger_raw()->log(in_location, spdlog::level::err, in_status.errorString());
      throw maya_UnknownParameter{in_status.errorString()};
    }
    case MStatus::MStatusCode::kNotImplemented: {
      spdlog::default_logger_raw()->log(in_location, spdlog::level::err, in_status.errorString());
      throw maya_NotImplemented{in_status.errorString()};
    }
    case MStatus::MStatusCode::kNotFound: {
      spdlog::default_logger_raw()->log(in_location, spdlog::level::err, in_status.errorString());
      throw maya_NotFound{in_status.errorString()};
    }
    case MStatus::MStatusCode::kEndOfFile: {
      spdlog::default_logger_raw()->log(in_location, spdlog::level::err, in_status.errorString());
      throw maya_EndOfFile{in_status.errorString()};
    }
    default:
      throw doodle_error{"未知选项"};
  }
}
}  // namespace doodle::maya_plug
