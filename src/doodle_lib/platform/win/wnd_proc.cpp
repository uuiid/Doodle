//
// Created by TD on 2022/1/18.
//

#include "wnd_proc.h"
#include <gui/main_proc_handle.h>
// Helper functions
#include <Windows.h>
#include <d3d11.h>
#include <tchar.h>
#include <shellapi.h>
#include <imgui_impl_win32.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace doodle::win {

d3d_device* d3d_device::self = nullptr;

bool d3d_device::CreateDeviceD3D(HWND hWnd) {
  // Setup swap chain
  DXGI_SWAP_CHAIN_DESC sd;
  ZeroMemory(&sd, sizeof(sd));
  sd.BufferCount                        = 2;
  sd.BufferDesc.Width                   = 0;
  sd.BufferDesc.Height                  = 0;
  sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.BufferDesc.RefreshRate.Numerator   = 60;
  sd.BufferDesc.RefreshRate.Denominator = 1;
  sd.Flags                              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
  sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.OutputWindow                       = hWnd;
  sd.SampleDesc.Count                   = 1;
  sd.SampleDesc.Quality                 = 0;
  sd.Windowed                           = TRUE;
  sd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;

  UINT createDeviceFlags                = 0;
  // createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
  D3D_FEATURE_LEVEL featureLevel;
  const D3D_FEATURE_LEVEL featureLevelArray[2] = {
      D3D_FEATURE_LEVEL_11_0,
      D3D_FEATURE_LEVEL_10_0,
  };
  if (D3D11CreateDeviceAndSwapChain(nullptr,
                                    D3D_DRIVER_TYPE_HARDWARE,
                                    nullptr,
                                    createDeviceFlags,
                                    featureLevelArray,
                                    2,
                                    D3D11_SDK_VERSION,
                                    &sd,
                                    &g_pSwapChain,
                                    &g_pd3dDevice,
                                    &featureLevel,
                                    &g_pd3dDeviceContext) != S_OK)
    return false;

  CreateRenderTarget();
  return true;
}

void d3d_device::CleanupDeviceD3D() {
  CleanupRenderTarget();
  if (g_pSwapChain) {
    g_pSwapChain->Release();
    g_pSwapChain = nullptr;
  }
  if (g_pd3dDeviceContext) {
    g_pd3dDeviceContext->Release();
    g_pd3dDeviceContext = nullptr;
  }
  if (g_pd3dDevice) {
    g_pd3dDevice->Release();
    g_pd3dDevice = nullptr;
  }
}

void d3d_device::CreateRenderTarget() {
  ID3D11Texture2D* pBackBuffer;
  g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
  g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
  pBackBuffer->Release();
}

void d3d_device::CleanupRenderTarget() {
  if (g_mainRenderTargetView) {
    g_mainRenderTargetView->Release();
    g_mainRenderTargetView = nullptr;
  }
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    return true;

  switch (msg) {
    case WM_SIZE:
      if (d3d_device::Get().g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
        d3d_device::Get().CleanupRenderTarget();
        d3d_device::Get().g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
        d3d_device::Get().CreateRenderTarget();
      }
      return 0;
    case WM_SYSCOMMAND:
      if ((wParam & 0xfff0) == SC_KEYMENU)  // Disable ALT application menu
        return 0;
      break;
    case WM_DPICHANGED:
      if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports) {
        // const int dpi = HIWORD(wParam);
        // printf("WM_DPICHANGED to %d (%.0f%%)\n", dpi, (float)dpi / 96.0f * 100.0f);
        const RECT* suggested_rect = (RECT*)lParam;
        ::SetWindowPos(hWnd, nullptr, suggested_rect->left, suggested_rect->top, suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
      }
      break;
    case WM_CLOSE: {
      doodle::gui::main_proc_handle::get().win_close();
      return 0;
    }
    case WM_DESTROY: {
      doodle::gui::main_proc_handle::get().win_destroy();
      return 0;
    }
      //    case WM_IME_CHAR: {
      //      auto& io    = ImGui::GetIO();
      //      DWORD wChar = wParam;
      //      if (wChar <= 127) {
      //        io.AddInputCharacter(wChar);
      //      } else {
      //        // swap lower and upper part.
      //        BYTE low  = (BYTE)(wChar & 0x00FF);
      //        BYTE high = (BYTE)((wChar & 0xFF00) >> 8);
      //        wChar     = MAKEWORD(high, low);
      //        wchar_t ch[6];
      //        MultiByteToWideChar(CP_OEMCP, 0, (LPCSTR)&wChar, 4, ch, 3);
      //        io.AddInputCharacter(ch[0]);
      //      }
      //    }
    default:
      break;
  }
  return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

d3d_device::d3d_device(wnd_handle const& in_handle) {
  handle_wnd = in_handle;
  CreateDeviceD3D(in_handle);
  self = this;
}
d3d_device::~d3d_device() {
  CleanupDeviceD3D();
}
d3d_device& d3d_device::Get() {
  return *self;
}
}  // namespace doodle::win
