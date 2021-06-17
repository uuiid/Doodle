#pragma once

#include "DoodleLib_fwd.h"
#include <stdexcept>

DOODLE_NAMESPACE_S
// 空指针错误
class DOODLELIB_API DoodleError : public std::runtime_error {
 public:
  explicit DoodleError(const std::string& message) : std::runtime_error(message){};
};
class DOODLELIB_API nullptr_error : public DoodleError {
 public:
  explicit nullptr_error(const std::string &err) : DoodleError(err){};
};
// fileErr
class DOODLELIB_API FileError : public DoodleError {
  FSys::path p_path;

 public:
  FileError(FSys::path path, const std::string& message)
      : DoodleError(path.generic_string().append(message)),
       p_path(std::move(path)){};
};
//doodl err

DOODLE_NAMESPACE_E
