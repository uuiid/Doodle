#include "Exception.h"

DOODLE_NAMESPACE_S

const char *not_file_error::what() const noexcept {
  std::string str("file not exists : ");
  str = str + std::runtime_error::what();
  return str.c_str();
}

const char *copy_error::what() const noexcept {
  std::string str("copy file failed : ");
  str = str + std::runtime_error::what();
  return str.c_str();
}

const char *upload_error::what() const noexcept {
  std::string str("not upload file : ");
  str = str + std::runtime_error::what();
  return str.c_str();
}

const char *insert_error_info::what() const noexcept {
  std::string str("insert err info :");
  str = str + std::runtime_error::what();
  return str.c_str();
}

const char *find_error_info::what() const noexcept {
  std::string str("not find err info :");
  str = str + std::runtime_error::what();
  return str.c_str();
}

const char *nullptr_error::what() const noexcept {
  std::string str("nullptr err : ");
  str = str + std::runtime_error::what();
  return str.c_str();
}

DOODLE_NAMESPACE_E