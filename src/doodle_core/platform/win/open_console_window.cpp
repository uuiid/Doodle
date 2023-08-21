//
// Created by td_main on 2023/8/21.
//
#include <doodle_core/platform/win/windows_alias.h>

#include <boost/locale.hpp>

#include <locale>
#include <wil/result.h>
namespace doodle::win {

void open_console_window() {
  if (::GetConsoleWindow() == nullptr) {
    THROW_IF_WIN32_BOOL_FALSE(::AllocConsole());
    FILE* l_file{};
    ::freopen_s(&l_file, "CONIN$", "r", stdin);
    ::freopen_s(&l_file, "CONOUT$", "w", stdout);
    ::freopen_s(&l_file, "CONERR$", "w", stderr);
    ::SetFocus(::GetConsoleWindow());

    std::locale::global(boost::locale::generator().generate("UTF-8"));
    THROW_IF_WIN32_BOOL_FALSE(::SetConsoleOutputCP(65001));
  }
}
}  // namespace doodle::win