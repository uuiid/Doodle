#pragma once

#include "core_global.h"
#include <stdexcept>

DOODLE_NAMESPACE_S

//----------------------------------------------------------------
class CORE_API doodle_notFile : public std::runtime_error {
 public:
  doodle_notFile(const std::string &err) : std::runtime_error(err){};
  virtual const char *what() const noexcept override;
};

//----------------------------------------------------------------
class CORE_API doodle_CopyErr : public std::runtime_error {
 public:
  doodle_CopyErr(const std::string &err) : std::runtime_error(err){};
  virtual const char *what() const noexcept override;
};

//----------------------------------------------------------------
class CORE_API doodle_upload_error : public std::runtime_error {
 public:
  doodle_upload_error(const std::string &err) : std::runtime_error(err){};
  virtual const char *what() const noexcept override;
};

//----------------------------------------------------------------
class CORE_API doodle_InsertErrorInfo : public std::runtime_error {
 public:
  doodle_InsertErrorInfo(const std::string &err) : std::runtime_error(err){};
  virtual const char *what() const noexcept override;
};

class CORE_API doodle_FindErrorInfo : public std::runtime_error {
 public:
  doodle_FindErrorInfo(const std::string &err) : std::runtime_error(err){};
  virtual const char *what() const noexcept override;
};

class doodle_nullptr : public std::runtime_error {
 public:
  doodle_nullptr(const std::string &err) : std::runtime_error(err){};
  virtual const char *what() const noexcept override;
};

DOODLE_NAMESPACE_E