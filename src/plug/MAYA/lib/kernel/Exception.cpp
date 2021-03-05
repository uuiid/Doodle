#include <lib/kernel/Exception.h>
namespace doodle::motion {
const char* FbxFileError::what() const noexcept {
  std::string str("fbx error : ");
  str = str + std::runtime_error::what();
  return str.c_str();
}
}  // namespace doodle::motion