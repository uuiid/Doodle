//
// Created by TD on 24-8-23.
//
#include <boost/exception/diagnostic_information.hpp>

#include <Windows.h>
std::wstring to_wstr(const std::string& in_str) {
  std::wstring l_ret{};
  int convertResult = ::MultiByteToWideChar(CP_UTF8, 0, in_str.c_str(), (int)strlen(in_str.c_str()), NULL, 0);
  if (convertResult > 0) {
    l_ret.resize(convertResult);
    convertResult =
        ::MultiByteToWideChar(CP_UTF8, 0, in_str.c_str(), (int)strlen(in_str.c_str()), &l_ret[0], l_ret.size());
    if (convertResult <= 0) {
      l_ret = TEXT("未知编码,错误消息无法显示");
    }
  } else {
    l_ret = TEXT("未知编码,错误消息无法显示");
  }
  return l_ret;
}

void show_error(const std::string& in_str) {
  ::MessageBoxW(nullptr, to_wstr(in_str).c_str(), L"错误", MB_OK | MB_ICONERROR);
}

extern "C" int main(int argc, const char* const argv[]) try {

} catch (...) {
  show_error(boost::current_exception_diagnostic_information());
}
