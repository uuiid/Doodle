#pragma once

#include <lib/MotionGlobal.h>

namespace doodle::motion {

//fbx文件导出错误
class FbxFileError : public std::runtime_error {
 public:
  FbxFileError(const std::string &err) : std::runtime_error(err){};
  virtual const char *what() const noexcept override;
};

class MayaNullptrError : public std::runtime_error {
 public:
  MayaNullptrError(const std::string &err) : std::runtime_error(err){};
  virtual const char *what() const noexcept override;
};

class MayaError : public std::runtime_error {
 public:
  MayaError(const std::string &err) : std::runtime_error(err){};
  virtual const char *what() const noexcept override;
};

class FFmpegError : public std::runtime_error {
 public:
  FFmpegError(const std::string &err) : std::runtime_error(err){};
  virtual const char *what() const noexcept override;
};
}  // namespace doodle::motion