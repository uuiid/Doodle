#pragma once

#include "pinyinlib/pinyin_global.h"

PINYIN_NAMESPACE_S

class PINYIN_EXPORT convert {
 public:
  std::string toEn(const std::string &conStr);
  std::string toEn(const std::wstring &conStr);
  std::string toEn(const wchar_t &conStr);
  static convert &Get() noexcept;

  convert &operator=(const convert &) = delete;
  convert(const convert &)            = delete;

 private:
  convert();
  ~convert();
};

DNAMESPACE_E
