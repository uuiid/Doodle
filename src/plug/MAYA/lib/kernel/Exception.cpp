#include <lib/kernel/Exception.h>
namespace doodle::motion {
const char* FbxFileError::what() const noexcept {
  std::string str("fbx error : ");
  str = str + std::runtime_error::what();
  return str.c_str();
}

const char* MayaNullptrError::what() const noexcept {
  std::string str("maya nullptr error : ");
  str = str + std::runtime_error::what();
  return str.c_str();
}

const char* MayaError::what() const noexcept {
  std::string str("maya  error : ");
  str = str + std::runtime_error::what();
  return str.c_str();
}

const char* FFmpegError::what() const noexcept {
  std::string str("ffpeg  error : ");
  str = str + std::runtime_error::what();
  return str.c_str();
}
}  // namespace doodle::motion