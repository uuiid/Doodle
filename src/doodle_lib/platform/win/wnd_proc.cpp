//
// Created by TD on 2022/1/18.
//

#include "wnd_proc.h"

// Helper functions
#include <d3d11.h>
#include <tchar.h>
#include <shellapi.h>
#include <imgui_impl_win32.h>
#include <doodle_lib/core/app_base.h>
#include <doodle_lib/app/app.h>

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
    case WM_DESTROY: {
      doodle::app_base::Get().post_quit_message();
      return 0;
    }
    case WM_DPICHANGED:
      if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports) {
        // const int dpi = HIWORD(wParam);
        // printf("WM_DPICHANGED to %d (%.0f%%)\n", dpi, (float)dpi / 96.0f * 100.0f);
        const RECT* suggested_rect = (RECT*)lParam;
        ::SetWindowPos(hWnd, nullptr, suggested_rect->left, suggested_rect->top, suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
      }
      break;
    case WM_CLOSE: {
      //      doodle::doodle_app::Get()->p_done = true;
      doodle::app_base::Get().stop_app();
      doodle::app::Get().hide_windows();
      ::DestroyWindow(doodle::app::Get().p_hwnd);
      return 0;
    }
      //    case WM_DROPFILES: {
      //      const auto hdrop = reinterpret_cast<::HDROP>(wParam);
      //      auto file_size   = DragQueryFile(hdrop, 0xFFFFFFFF, nullptr, 0);
      //      std::vector<doodle::FSys::path> l_vector{};
      //
      //      //我们可以同时拖动多个文件，所以我们必须在这里循环
      //      for (UINT i = 0; i < file_size; i++) {
      //        std::size_t l_len = DragQueryFile(hdrop, i, nullptr, 0) + 1;
      //        std::unique_ptr<wchar_t[]> varbuf{new wchar_t[l_len]};
      //
      //        UINT cch = DragQueryFile(hdrop, i, varbuf.get(), l_len);
      //        doodle::chick_true<doodle::doodle_error>(cch != 0, DOODLE_LOC, "拖拽文件获取失败");
      //        l_vector.emplace_back(varbuf.get());
      //      }
      //      DragFinish(hdrop);
      //      DOODLE_LOG_INFO("查询到文件拖拽 :\n{}", fmt::join(l_vector, "\n"));
      //
      //      break;
      //    }
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
cv::Rect2f get_system_metrics_VIRTUALSCREEN() {
  //  int screenx = GetSystemMetrics(SM_XVIRTUALSCREEN);
  //  int screeny = GetSystemMetrics(SM_YVIRTUALSCREEN);
  //  int width   = GetSystemMetrics(SM_CXVIRTUALSCREEN);
  //  int height  = GetSystemMetrics(SM_CYVIRTUALSCREEN);
  return cv::Rect{GetSystemMetrics(SM_XVIRTUALSCREEN),
                  GetSystemMetrics(SM_YVIRTUALSCREEN),
                  GetSystemMetrics(SM_CXVIRTUALSCREEN),
                  GetSystemMetrics(SM_CYVIRTUALSCREEN)};
}

namespace {
BITMAPINFOHEADER createBitmapHeader(int width, int height) {
  BITMAPINFOHEADER bi;

  // create a bitmap
  bi.biSize          = sizeof(BITMAPINFOHEADER);
  bi.biWidth         = width;
  bi.biHeight        = -height;  // this is the line that makes it draw upside down or not
  bi.biPlanes        = 1;
  bi.biBitCount      = 32;
  bi.biCompression   = BI_RGB;
  bi.biSizeImage     = 0;
  bi.biXPelsPerMeter = 0;
  bi.biYPelsPerMeter = 0;
  bi.biClrUsed       = 0;
  bi.biClrImportant  = 0;

  return bi;
}
}  // namespace
cv::Mat get_screenshot() {
  auto hwnd = GetDesktopWindow();
  cv::Mat src{};
  // get handles to a device context (DC)
  HDC hwindowDC           = GetDC(hwnd);
  HDC hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
  SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);
  // define scale, height and width
  auto k_rot = win::get_system_metrics_VIRTUALSCREEN();

  // create mat object
  src.create(k_rot.height, k_rot.width, CV_8UC4);

  // create a bitmap
  HBITMAP hbwindow    = CreateCompatibleBitmap(hwindowDC, k_rot.width, k_rot.height);
  BITMAPINFOHEADER bi = createBitmapHeader(k_rot.width, k_rot.height);

  // use the previously created device context with the bitmap
  SelectObject(hwindowCompatibleDC, hbwindow);

  // copy from the window device context to the bitmap device context
  StretchBlt(hwindowCompatibleDC, 0, 0, k_rot.width, k_rot.height,
             hwindowDC, k_rot.x, k_rot.y, k_rot.width, k_rot.height,
             SRCCOPY);  // change SRCCOPY to NOTSRCCOPY for wacky colors !
  GetDIBits(hwindowCompatibleDC, hbwindow,
            0, k_rot.height, src.data,
            (BITMAPINFO*)&bi, DIB_RGB_COLORS);  // copy from hwindowCompatibleDC to hbwindow

  // avoid memory leak
  DeleteObject(hbwindow);
  DeleteDC(hwindowCompatibleDC);
  ReleaseDC(hwnd, hwindowDC);
  return src;
}
#if 0
std::string get_font_data() {
  auto dc = ::GetDC(d3d_device::Get().handle_wnd);
  LOGFONTW logfont{};
  logfont.lfCharSet        = CHINESEBIG5_CHARSET;
  logfont.lfFaceName[0]    = '\0';
  logfont.lfPitchAndFamily = 0;

  LOGFONTW logfont_init{};

  std::string str{};
  ::EnumFontFamiliesExW(
      dc, &logfont,
      [](const LOGFONT* lpelfe,
         const TEXTMETRIC* lpntme,
         DWORD FontType,
         LPARAM lParam) {
        auto* font = reinterpret_cast<LOGFONTW*>(lParam);
        switch (FontType) {
          case DEVICE_FONTTYPE:
            break;

          case RASTER_FONTTYPE:
            break;

          case TRUETYPE_FONTTYPE:
            *font = *lpelfe;
            return 0;
            break;
          default:
            break;
        }
        return 1;
      },
      reinterpret_cast<LPARAM>(&logfont_init), 0);

  auto hfont = ::CreateFontIndirectW(&logfont_init);
  chick_true<doodle_error>(hfont, DOODLE_LOC, "无法获取字体数据");
  ::SelectObject(dc, hfont);

  LPVOID ptr      = NULL;
  HGLOBAL hGlobal = NULL;

  auto l_size     = ::GetFontData(dc, 0, 0, nullptr, 0);
  chick_true<doodle_error>(l_size != GDI_ERROR, DOODLE_LOC, "无法获取字体数据");
  std::unique_ptr<char[]> l_buff{new char[l_size]};

  hGlobal   = GlobalAlloc(GMEM_MOVEABLE, l_size);
  ptr       = GlobalLock(hGlobal);

  auto l_r_ = ::GetFontData(dc, 0, 0, ptr, l_size);
  chick_true<doodle_error>(l_r_ != GDI_ERROR, DOODLE_LOC, "无法获取字体数据");
  IStream* fontStream = NULL;
  l_r_                = CreateStreamOnHGlobal(hGlobal, TRUE, &fontStream);
  ULONG _l_long{};
  fontStream->Read(l_buff.get(),l_size,&_l_long);

  std::string l_r{};
  std::copy_n(l_buff.get(), l_size, std::back_inserter(l_r));
  GlobalUnlock(hGlobal);
  return l_r;
}
#endif

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
