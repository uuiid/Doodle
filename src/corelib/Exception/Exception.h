#pragma once

#include "corelib/core_global.h"
#include <stdexcept>

DOODLE_NAMESPACE_S
// 空指针错误
class CORE_API nullptr_error : public std::runtime_error {
 public:
  nullptr_error(const std::string &err) : std::runtime_error(err){};
};
// fileErr
class CORE_API FileError : public std::runtime_error {
  FSys::path p_path;

 public:
  FileError(FSys::path path, std::string message) : std::runtime_error(message), p_path(std::move(path)){};
};
//doodl err
class CORE_API DoodleError : public std::runtime_error {
 public:
  DoodleError(std::string message) : std::runtime_error(message){};
};
//-------------------------没有文件错误-----------------------------
class CORE_API not_file_error : public std::runtime_error {
 public:
  not_file_error(const std::string &err) : std::runtime_error(err){};
};

//-------------------------复制错误-------------------------------
class CORE_API copy_error : public std::runtime_error {
 public:
  copy_error(const std::string &err) : std::runtime_error(err){};
};

//------------------------无法上传错误--------------------------------
class CORE_API upload_error : public std::runtime_error {
 public:
  upload_error(const std::string &err) : std::runtime_error(err){};
};

//-----------------------数据库插入错误-----------------------------
class CORE_API insert_error_info : public std::runtime_error {
 public:
  insert_error_info(const std::string &err) : std::runtime_error(err){};
};

//寻找不到错误
class CORE_API find_error_info : public std::runtime_error {
 public:
  find_error_info(const std::string &err) : std::runtime_error(err){};
};

//-------------------------rttr 反射没有类型错误---------------
class CORE_API rttr_error : public std::runtime_error {
 public:
  rttr_error(const std::string &err) : std::runtime_error(err){};
};

class CORE_API rttr_not_class : public rttr_error {
 public:
  rttr_not_class(const std::string &err) : rttr_error(err){};
};

class CORE_API rttr_method_invoke_class : public rttr_error {
 public:
  rttr_method_invoke_class(const std::string &err) : rttr_error(err){};
};
DOODLE_NAMESPACE_E