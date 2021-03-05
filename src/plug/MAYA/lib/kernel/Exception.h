#pragma once

#include <lib/MotionGlobal.h>

namespace doodle::motion {

//fbx文件导出错误
class FbxFileError : public std::runtime_error {
 public:
  FbxFileError(const std::string &err) : std::runtime_error(err){};
  virtual const char *what() const noexcept override;
};
}