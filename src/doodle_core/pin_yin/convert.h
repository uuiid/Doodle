#pragma once

#include <doodle_core/doodle_core_fwd.h>

namespace doodle {
class DOODLE_CORE_API convert {
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
}  // namespace doodle
