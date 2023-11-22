//
// Created by td_main on 2023/8/21.
//
#include <doodle_core/platform/win/windows_alias.h>

#include <boost/locale.hpp>

#include <Windows.h>
#include <locale>
#include <ole2.h>
#include <wil/registry.h>
#include <wil/resource.h>
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

std::string get_clipboard_data_str() {
  THROW_IF_WIN32_BOOL_FALSE(::OpenClipboard(nullptr));
  auto* l_handle = ::GetClipboardData(CF_UNICODETEXT);
  if (l_handle == nullptr) {
    THROW_LAST_ERROR();
  }
  wil::unique_hglobal_locked const l_lock{l_handle};
  auto l_r = boost::locale::conv::utf_to_utf<char>(static_cast<wchar_t*>(l_handle));
  THROW_IF_WIN32_BOOL_FALSE(::CloseClipboard());
  return l_r;
  //  return boost::locale::conv::utf_to_utf<char>(l_str);
}

}  // namespace doodle::win