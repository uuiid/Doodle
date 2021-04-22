#pragma once

#include "corelib/core_global.h"
#include <stdexcept>

DOODLE_NAMESPACE_S
// 空指针错误
class CORE_API DoodleError : public std::runtime_error {
 public:
  DoodleError(std::string message) : std::runtime_error(message){};
};
class CORE_API nullptr_error : public DoodleError {
 public:
  nullptr_error(const std::string &err) : DoodleError(err){};
};
// fileErr
class CORE_API FileError : public DoodleError {
  FSys::path p_path;

 public:
  FileError(FSys::path path, std::string message)
      : DoodleError(path.generic_string().append(message)),
       p_path(std::move(path)){};
};
//doodl err

DOODLE_NAMESPACE_E