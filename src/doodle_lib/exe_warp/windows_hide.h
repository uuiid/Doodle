//
// Created by TD on 24-7-19.
//

#pragma once
#include <windows.h>
#include <boost/process/v2.hpp>
#include <boost/winapi/show_window.hpp>
#include <boost/winapi/process.hpp>

namespace doodle::details {
struct hide_and_not_create_windows_t {
  constexpr hide_and_not_create_windows_t() = default;

  // template <typename Launcher>
  boost::process::v2::error_code on_setup(boost::process::v2::windows::default_launcher& launcher,
                                          const boost::process::v2::filesystem::path&, const std::wstring&) const {
    launcher.startup_info.StartupInfo.dwFlags |= ::boost::winapi::STARTF_USESHOWWINDOW_;
    launcher.startup_info.StartupInfo.wShowWindow |= ::boost::winapi::SW_HIDE_;
    launcher.creation_flags |= ::boost::winapi::CREATE_NO_WINDOW_;
    return boost::process::v2::error_code{};
  }
};

constexpr static hide_and_not_create_windows_t hide_and_not_create_windows{};
}