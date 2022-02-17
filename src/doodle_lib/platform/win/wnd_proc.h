//
// Created by TD on 2022/1/18.
//

#pragma once
#include <doodle_lib/platform/win/windows_alias.h>
#include <eigen3/Eigen/Core>
#include <opencv2/core.hpp>

namespace doodle::win {

class DOODLELIB_API d3d_device {
  static d3d_device* self;

 public:
  ID3D11Device* g_pd3dDevice                     = nullptr;
  ID3D11DeviceContext* g_pd3dDeviceContext       = nullptr;
  IDXGISwapChain* g_pSwapChain                   = nullptr;
  ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
  wnd_handle handle_wnd;

  explicit d3d_device(const wnd_handle& in_handle);
  virtual ~d3d_device();
  bool CreateDeviceD3D(HWND hWnd);
  void CleanupDeviceD3D();

  void CreateRenderTarget();
  void CleanupRenderTarget();
  static d3d_device& Get();
};

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
/**
 * @brief 返回虚拟屏幕的坐标
 *
 * @return 左上角x,左上角y, 右下角x,右下角y
 * 0 GetSystemMetrics(SM_XVIRTUALSCREEN);
 * 1 GetSystemMetrics(SM_YVIRTUALSCREEN);
 * 2 GetSystemMetrics(SM_CXVIRTUALSCREEN);
 * 3 GetSystemMetrics(SM_CYVIRTUALSCREEN);
 */
cv::Rect2f get_system_metrics_VIRTUALSCREEN();
cv::Mat get_screenshot();



}  // namespace doodle::win
