//
// Created by TD on 2021/9/14.
//

#include "doodle_app.h"
#include <doodle_lib/exception/exception.h>
#include <doodle_lib/logger/logger.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/gui/main_windwos.h>
#include <doodle_lib/gui/widget_register.h>
#include <doodle_lib/metadata/metadata.h>
#include <doodle_lib/platform/win/drop_manager.h>

// Helper functions
#include <d3d11.h>
#include <tchar.h>
#include <shellapi.h>

// Data
static ID3D11Device* g_pd3dDevice                     = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext       = nullptr;
static IDXGISwapChain* g_pSwapChain                   = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool CreateDeviceD3D(HWND hWnd) {
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
  if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
    return false;

  CreateRenderTarget();
  return true;
}

void CleanupDeviceD3D() {
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

void CreateRenderTarget() {
  ID3D11Texture2D* pBackBuffer;
  g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
  g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
  pBackBuffer->Release();
}

void CleanupRenderTarget() {
  if (g_mainRenderTargetView) {
    g_mainRenderTargetView->Release();
    g_mainRenderTargetView = nullptr;
  }
}

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0  // From Windows SDK 8.1+ headers
#endif

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    return true;

  switch (msg) {
    case WM_SIZE:
      if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
        CleanupRenderTarget();
        g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
        CreateRenderTarget();
      }
      return 0;
    case WM_SYSCOMMAND:
      if ((wParam & 0xfff0) == SC_KEYMENU)  // Disable ALT application menu
        return 0;
      break;
    case WM_DESTROY: {
      doodle::doodle_app::Get()->p_done = true;
      doodle::core_set::getSet().p_stop = true;
      doodle::core_set::getSet().p_condition.notify_all();
      // ::PostQuitMessage(0);
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
      doodle::doodle_app::Get()->p_done = true;
      doodle::core_set::getSet().p_stop = true;
      doodle::core_set::getSet().p_condition.notify_all();
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

#include <fmt/core.h>
#include <toolkit/toolkit.h>
#include <windows.h>
namespace doodle {
using win_handle = HWND;
using win_class  = WNDCLASSEX;

class doodle_app::impl {
 public:
  win_handle p_hwnd;
  win_class p_win_class;
  win::drop_manager p_drop_manager;
  impl()
      : p_hwnd(),
        p_win_class({sizeof(WNDCLASSEX),
                     CS_CLASSDC, WndProc,
                     0L,
                     0L,
                     GetModuleHandle(nullptr),
                     nullptr,
                     nullptr,
                     nullptr,
                     nullptr,
                     _T("doodle"),
                     nullptr}) {}
};

doodle_app* doodle_app::self;

doodle_app::doodle_app()
    : p_title(conv::utf_to_utf<wchar_t>(fmt::format(
          "doodle {}.{}.{}.{}",
          Doodle_VERSION_MAJOR,
          Doodle_VERSION_MINOR,
          Doodle_VERSION_PATCH,
          Doodle_VERSION_TWEAK))),
      p_done(false),
      wregister(new_object<widget_register>()),
      p_show_err(false),
      p_main_win(),
      p_pd3dDevice(nullptr),
      p_impl(std::make_unique<impl>()) {
  // Create application window
  // ImGui_ImplWin32_EnableDpiAwareness();
  ::RegisterClassEx(&p_impl->p_win_class);
  p_impl->p_hwnd = ::CreateWindow(p_impl->p_win_class.lpszClassName,
                                  p_title.c_str(),
                                  WS_OVERLAPPEDWINDOW,
                                  100, 100, 1280, 800,
                                  nullptr, nullptr,
                                  p_impl->p_win_class.hInstance,
                                  nullptr);

  // Initialize Direct3D
  if (!CreateDeviceD3D(p_impl->p_hwnd)) {
    CleanupDeviceD3D();
    ::UnregisterClass(p_impl->p_win_class.lpszClassName, p_impl->p_win_class.hInstance);
  }

  // Show the window
  ::ShowWindow(p_impl->p_hwnd, SW_SHOWDEFAULT);
  ::UpdateWindow(p_impl->p_hwnd);

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();

  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;    // Enable Docking
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;  // Enable Multi-Viewport / Platform Windows
  // io.ConfigViewportsNoAutoMerge = true;
  // io.ConfigViewportsNoTaskBarIcon = true;
  // io.ConfigViewportsNoDefaultParent = true;
  // io.ConfigDockingAlwaysTabBar = true;
  // io.ConfigDockingTransparentPayload = true;
  // io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;     // FIXME-DPI: Experimental. THIS CURRENTLY DOESN'T WORK AS EXPECTED. DON'T USE IN USER APP!
  // io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports; // FIXME-DPI: Experimental.

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  // ImGui::StyleColorsClassic();

  // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
  ImGuiStyle& style = ImGui::GetStyle();
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    style.WindowRounding              = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  }

  // Setup Platform/Renderer backends
  ImGui_ImplWin32_Init(p_impl->p_hwnd);
  ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
  p_pd3dDevice = g_pd3dDevice;

  /// 初始化文件拖拽
  //  DragAcceptFiles(p_impl->p_hwnd, true);
  //  OleInitialize(nullptr);
  //  auto k_r = RegisterDragDrop(p_impl->p_hwnd, new win::drop_manager{});
  //  chick_true<doodle_error>(k_r == S_OK, DOODLE_LOC, "无法注册拖拽com");
  /// \brief 设置本地静态变量
  doodle_app::self = this;
}
doodle_app::~doodle_app() {
  // Cleanup
  ImGui_ImplDX11_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();

  CleanupDeviceD3D();
  ::DestroyWindow(p_impl->p_hwnd);
  ::UnregisterClass(p_impl->p_win_class.lpszClassName, p_impl->p_win_class.hInstance);
//  OleUninitialize();
}

base_widget_ptr doodle_app::get_main_windows() const {
  return new_object<main_windows>();
}

void doodle_app::set_imgui_dock_space(const FSys::path& in_path) const {
  auto k_f = cmrc::DoodleLibResource::get_filesystem().open("resource/imgui.ini");
  if (FSys::exists(in_path))
    return;
  FSys::ofstream l_ofs{in_path, std::ios::out | std::ios::binary};
  if (l_ofs)
    l_ofs.write(k_f.begin(), k_f.size());
}

std::int32_t doodle_app::run() {
  loop_begin();
  while (!p_done) {
    loop_one();
  }
  hide_windows();
  return 0;
}

doodle_app* doodle_app::Get() {
  return doodle_app::self;
}

bool doodle_app::valid() const {
  return this->p_impl->p_hwnd != nullptr;
}
void doodle_app::metadata_load() {
  auto k_f = doodle_lib::Get().get_metadata_factory();
  for (auto& k_i : k_metadata_obs) {
    auto k_h = make_handle(k_i);
    if (k_h.get<database_stauts>().is<need_load>()) {
      k_f->select_indb(k_h);
    } else if (k_h.get<database_stauts>().is<need_save>()) {
      k_f->insert_into(k_h);
    } else if (k_h.get<database_stauts>().is<need_delete>()) {
      k_f->delete_data(k_h);
    } else if (k_h.get<database_stauts>().is<need_root_load>()) {
      k_f->select_indb_by_root(k_h);
    }
  }
  k_metadata_obs.clear();
}
void doodle_app::metadata_save() const {
}
void doodle_app::metadata_delete() const {
}

void doodle_app::metadata_loop_one() {
  metadata_save();
  metadata_load();
  metadata_delete();
}

base_widget_ptr doodle_app::loop_begin() {
  ::ShowWindow(p_impl->p_hwnd, SW_SHOW);
  // Load Fonts
  // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
  // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
  // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
  // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
  // - Read 'docs/FONTS.md' for more instructions and details.
  // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
  // io.Fonts->AddFontDefault();
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
  // ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
  // IM_ASSERT(font != NULL);
  static string imgui_file_path{(core_set::getSet().get_cache_root("imgui") / "imgui.ini").generic_string()};
  set_imgui_dock_space(imgui_file_path);

  ImGuiIO& io = ImGui::GetIO();
  io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\simkai.ttf)", 16.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
  io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\simhei.ttf)", 16.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
  io.IniFilename = imgui_file_path.c_str();
  k_metadata_obs.connect(*g_reg(), entt::collector.update<database_stauts>());
  p_main_win = get_main_windows();

  return p_main_win;
}

void doodle_app::hide_windows() {
  if (p_done)
    ::ShowWindow(p_impl->p_hwnd, SW_HIDE);
}

// Main loop
void doodle_app::loop_one() {
  // Poll and handle messages (inputs, window resize, etc.)
  // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
  // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
  // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
  // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
  MSG msg;
  while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
    if (msg.message == WM_QUIT)
      p_done = true;
  }
  if (p_done)
    return;

  // Start the Dear ImGui frame
  ImGui_ImplDX11_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();

  imgui::DockSpaceOverViewport(imgui::GetMainViewport());
  static std::string str{};
  static decltype(chrono::system_clock::now()) s_now{chrono::system_clock::now()};
  decltype(chrono::system_clock::now()) l_now{chrono::system_clock::now()};
  try {
    if (!p_show_err) {
      p_main_win->frame_render();
      main_loop();
      metadata_loop_one();
      g_main_loop().update(l_now - s_now, nullptr);
      g_bounded_pool().update(l_now - s_now, nullptr);
      s_now = l_now;
    }
  } catch (doodle_error& err) {
    DOODLE_LOG_ERROR("捕获 doodle_error异常 {}", err.what());
    p_show_err = true;
    str        = err.what();
    imgui::OpenPopup("警告");
  } catch (std::runtime_error& err) {
    DOODLE_LOG_ERROR("捕获 runtime_error异常 {}", err.what());
    p_show_err = true;
    str        = err.what();
    imgui::OpenPopup("警告");
  } catch (std::exception& err) {
    DOODLE_LOG_ERROR("捕获 exception异常 {}", err.what());
    p_show_err = true;
    str        = err.what();
    imgui::OpenPopup("警告");
  }
  dear::PopupModal{"警告", &p_show_err} && [str1 = str, this]() {
    dear::Text(str);
    if (ImGui::Button("OK")) {
      this->p_show_err = false;
      ImGui::CloseCurrentPopup();
    }
  };

  // Rendering
  ImGui::Render();
  ImVec4 clear_color                    = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
  const float clear_color_with_alpha[4] = {clear_color.x * clear_color.w,
                                           clear_color.y * clear_color.w,
                                           clear_color.z * clear_color.w,
                                           clear_color.w};
  g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
  g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
  ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

  // Update and Render additional Platform Windows
  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
  }

  g_pSwapChain->Present(1, 0);  // Present with vsync
                                // g_pSwapChain->Present(0, 0); // Present without vsync
}
}  // namespace doodle
