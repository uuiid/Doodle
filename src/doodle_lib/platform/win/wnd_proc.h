//
// Created by TD on 2022/1/18.
//

#pragma once
#include <doodle_lib/platform/win/windows_alias.h>

namespace doodle::win {

class DOODLELIB_API d3d_device {
  static d3d_device* self;

 public:
  ID3D11Device* g_pd3dDevice                     = nullptr;
  ID3D11DeviceContext* g_pd3dDeviceContext       = nullptr;
  IDXGISwapChain* g_pSwapChain                   = nullptr;
  ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

  explicit d3d_device(const wnd_handle& in_handle);
  virtual ~d3d_device();
  bool CreateDeviceD3D(HWND hWnd);
  void CleanupDeviceD3D();

  void CreateRenderTarget();
  void CleanupRenderTarget();
  static d3d_device& Get();
};

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
}  // namespace doodle::win
