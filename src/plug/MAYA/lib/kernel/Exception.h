#pragma once

#include <lib/MotionGlobal.h>

namespace doodle::motion {
class DoodleError : public std::runtime_error {
 public:
  DoodleError(std::string message) : std::runtime_error(message){};
};

class MotionError : public std::runtime_error {
 public:
  MotionError(const std::string &err) : std::runtime_error(err){};
};

//fbx文件导出错误
class FbxFileError : public std::runtime_error {
 public:
  FbxFileError(const std::string &err) : std::runtime_error(err){};
};

class MayaNullptrError : public std::runtime_error {
 public:
  MayaNullptrError(const std::string &err) : std::runtime_error(err){};
};

class MayaError : public std::runtime_error {
 public:
  MayaError(const std::string &err) : std::runtime_error(err){};
};

class FFmpegError : public std::runtime_error {
 public:
  FFmpegError(const std::string &err) : std::runtime_error(err){};
};

class NotFileError : public std::runtime_error {
  FSys::path p_file;

 public:
  NotFileError(const FSys::path &err)
      : std::runtime_error(""),
        p_file(std::move(err)){};
};
}  // namespace doodle::motion