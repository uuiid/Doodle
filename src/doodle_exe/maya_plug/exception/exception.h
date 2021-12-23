//
// Created by TD on 2021/12/20.
//

#pragma once
#include <doodle_lib/exception/exception.h>
#include <doodle_lib/logger/logger.h>
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

inline void throw_maya_exception(const MStatus& in_status, const ::spdlog::source_loc& in_location) {
  switch (in_status.statusCode()) {
    case MStatus::MStatusCode::kSuccess:
      break;
    case MStatus::MStatusCode::kFailure: {
      ::spdlog::log(in_location, spdlog::level::err, in_status.errorString());
      chick_true<maya_Failure>(false, in_location, in_status.errorString());
    }
    case MStatus::MStatusCode::kInsufficientMemory: {
      ::spdlog::log(in_location, spdlog::level::err, in_status.errorString());
      chick_true<maya_InsufficientMemory>(false, in_location, in_status.errorString());
    }
    case MStatus::MStatusCode::kInvalidParameter: {
      ::spdlog::log(in_location, spdlog::level::err, in_status.errorString());
      chick_true<maya_InvalidParameter>(false, in_location, in_status.errorString());
    }
    case MStatus::MStatusCode::kLicenseFailure: {
      ::spdlog::log(in_location, spdlog::level::err, in_status.errorString());
      chick_true<maya_LicenseFailure>(false, in_location, in_status.errorString());
    }
    case MStatus::MStatusCode::kUnknownParameter: {
      ::spdlog::log(in_location, spdlog::level::err, in_status.errorString());
      chick_true<maya_UnknownParameter>(false, in_location, in_status.errorString());
    }
    case MStatus::MStatusCode::kNotImplemented: {
      ::spdlog::log(in_location, spdlog::level::err, in_status.errorString());
      chick_true<maya_NotImplemented>(false, in_location, in_status.errorString());
    }
    case MStatus::MStatusCode::kNotFound: {
      ::spdlog::log(in_location, spdlog::level::err, in_status.errorString());
      chick_true<maya_NotFound>(false, in_location, in_status.errorString());
    }
    case MStatus::MStatusCode::kEndOfFile: {
      ::spdlog::log(in_location, spdlog::level::err, in_status.errorString());
      chick_true<maya_EndOfFile>(false, in_location, in_status.errorString());
    }
    default:
      chick_true<doodle_error>(false, in_location, "未知选项");
  }
}
}  // namespace doodle::maya_plug
