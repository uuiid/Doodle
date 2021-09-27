#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <stdexcept>

namespace doodle {
// 空指针错误
class DOODLELIB_API doodle_error : public std::runtime_error {
 public:
  explicit doodle_error(const std::string& message) : std::runtime_error(message){};
};
class DOODLELIB_API nullptr_error : public doodle_error {
 public:
  explicit nullptr_error(const std::string &err) : doodle_error(err){};
};
// fileErr
class DOODLELIB_API file_error : public doodle_error {
  FSys::path p_path;

 public:
  file_error(FSys::path path, const std::string& message)
      : doodle_error(path.generic_string().append(message)),
       p_path(std::move(path)){};
};
//doodl err

}
