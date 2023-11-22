//
// Created by TD on 2022/1/18.
//

#pragma once

#include <Windows.h>
// #include <windef.h>
// #include <boost/winapi/handles.hpp>
// #include <boost/winapi/show_window.hpp>
#include <doodle_core/configure/doodle_core_export.h>

#include <string>
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11RenderTargetView;
namespace doodle::win {
using wnd_handle   = ::HWND;
using wnd_class    = ::WNDCLASSEX;
using wnd_instance = ::HINSTANCE;
class d3d_device;
using string_type = PWSTR;

void DOODLE_CORE_API open_console_window();

std::string DOODLE_CORE_API get_clipboard_data_str();

}  // namespace doodle::win
