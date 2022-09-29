//
// Created by TD on 2022/9/29.
//

#include "app.h"
#include <doodle_app/platform/win/windows_proc.h>
#include <doodle_core/core/file_sys.h>
#include <doodle_core/thread_pool/process_pool.h>
#include <doodle_app/lib_warp/imgui_warp.h>
#include <gui/main_menu_bar.h>
#include <gui/main_status_bar.h>
#include <doodle_core/platform/win/drop_manager.h>
#include <doodle_app/app/short_cut.h>
#include <doodle_app/gui/get_input_dialog.h>
#include <doodle_core/core/core_sig.h>
#include <doodle_core/gui_template/gui_process.h>
#include <doodle_core/core/init_register.h>
#include <gui/main_proc_handle.h>
#include <gui/get_input_dialog.h>

#include <doodle_app/app/program_options.h>
#include <doodle_app/lib_warp/icon_font_macro.h>
#include <boost/locale.hpp>

#include <implot.h>
#include <implot_internal.h>

// Helper functions
#include <d3d11.h>
#include <tchar.h>
// 启用窗口拖拽导入头文件
#include <shellapi.h>

#include <imgui.h>

namespace doodle {
/**
 * @brief 内部类
 *
 */
class doodle_main_app::impl {
 public:
  /// \brief 初始化 com
  [[maybe_unused]] win::ole_guard _guard{};
  win::wnd_handle parent{};
  std::int32_t show_enum{};

 public:
};

doodle_main_app::doodle_main_app(const in_gui_arg& in_arg)
    : app_command_base(in_arg),
      p_hwnd(),
      p_win_class(),
      d3d_deve(),
      p_show_err(false),
      d3dDevice(nullptr),
      d3dDeviceContext(nullptr),
      p_i(std::make_unique<impl>()) {
  p_i->parent    = in_arg.in_parent;
  p_i->show_enum = in_arg.show_enum;
}

void doodle_main_app::post_constructor() {
  app_command_base::post_constructor();
  p_win_class =
      {sizeof(WNDCLASSEX),
       CS_CLASSDC,
       win::WndProc,
       0L,
       0L,
       instance,
       nullptr, nullptr, nullptr, nullptr,
       _T("doodle"),
       nullptr};
  p_win_class.hIconSm = (HICON)LoadImageW(instance, MAKEINTRESOURCEW(1), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
  p_win_class.hIcon   = p_win_class.hIconSm;

  // Create application window
  // ImGui_ImplWin32_EnableDpiAwareness();
  ::RegisterClassExW(&p_win_class);
  p_hwnd = ::CreateWindowExW(0L, p_win_class.lpszClassName, p_title.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, p_i->parent, nullptr, p_win_class.hInstance, nullptr);

  // Initialize Direct3D
  g_reg()->ctx().emplace<std::shared_ptr<win::d3d_device>>(std::make_shared<win::d3d_device>(p_hwnd));

  // Show the window
  ::ShowWindow(p_hwnd, SW_SHOWDEFAULT);
  ::UpdateWindow(p_hwnd);
  ::ShowWindow(p_hwnd, p_i->show_enum);

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImPlot::CreateContext();
  ImGuiIO& io = ImGui::GetIO();

  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable Docking
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;    // Enable Multi-Viewport / Platform Windows
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
  ImGui_ImplWin32_Init(p_hwnd);
  ImGui_ImplDX11_Init(d3d_deve->g_pd3dDevice, d3d_deve->g_pd3dDeviceContext);
  d3dDevice        = d3d_deve->g_pd3dDevice;
  d3dDeviceContext = d3d_deve->g_pd3dDeviceContext;
  /// 启用文件拖拽
  DragAcceptFiles(p_hwnd, true);
  /// \brief 注册拖放对象
  auto k_r = RegisterDragDrop(p_hwnd, new win::drop_manager{});
  DOODLE_CHICK(k_r == S_OK, doodle_error{"无法注册拖拽com"});

  //  HMONITOR hmon  = MonitorFromWindow(p_impl->p_hwnd,
  //                                     MONITOR_DEFAULTTONEAREST);
  //  MONITORINFO mi = {sizeof(mi)};
  //  auto k_r       = GetMonitorInfo(hmon, &mi);
  //  DOODLE_CHICK(k_r != 0,doodle_error{ "无法设置全屏"});
  //  SetWindowPos(p_impl->p_hwnd, nullptr, mi.rcMonitor.left,
  //               mi.rcMonitor.top,
  //               mi.rcMonitor.right - mi.rcMonitor.left,
  //               mi.rcMonitor.bottom - mi.rcMonitor.top,
  //               SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

  auto imgui_file_path = core_set::get_set().get_cache_root("imgui") / "imgui.ini";
  static std::string _l_p{imgui_file_path.generic_string()};
  io.IniFilename = _l_p.c_str();

  //  ImGuiIO& io = ImGui::GetIO();
  //  io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\simkai.ttf)", 16.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
  {
    // io.Fonts->AddFontDefault();
    io.Fonts->AddFontFromFileTTF(doodle_config::font_default.data(), 16.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
    auto l_font                         = cmrc::DoodleLibResourceFont::get_filesystem().open("fa-solid-900.ttf");
    static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
    ImFontConfig icons_config;
    icons_config.MergeMode            = true;
    icons_config.PixelSnapH           = true;
    icons_config.FontDataOwnedByAtlas = false;
    // io.Fonts->AddFontFromFileTTF(FONT_ICON_FILE_NAME_FAS, 16.0f, &icons_config, icons_ranges);
    io.Fonts->AddFontFromMemoryTTF((void*)l_font.begin(), boost::numeric_cast<std::int32_t>(l_font.size()), 16.0f, &icons_config, icons_ranges);
  }

  g_reg()->ctx().at<core_sig>().init_end.connect([this]() {
    /// 在这里我们加载项目
    load_project(doodle_main_app::Get().options_ && !doodle_main_app::Get().options_->p_project_path.empty() ? doodle_main_app::Get().options_->p_project_path : core_set::get_set().project_root[0]);
    boost::asio::post(g_io_context(), [this]() { this->load_windows(); });
  });

  DOODLE_CHICK(::IsWindowUnicode(p_hwnd), doodle_error{"错误的窗口"});
  /// \brief 设置窗口句柄处理
  gui::main_proc_handle::get().win_close = []() {
    make_handle().emplace<gui::gui_windows>(std::make_shared<gui::close_exit_dialog>());
  };
  gui::main_proc_handle::get().win_destroy = [=]() {
    ::DestroyWindow(p_hwnd);
  };

  static std::function<void()> s_set_title_fun{};
  s_set_title_fun = [this]() {
    auto& l_prj  = g_reg()->ctx().at<project>();
    auto l_title = boost::locale::conv::utf_to_utf<char>(p_title);
    auto l_str   = fmt::format("{0} 文件 {1} 项目路径 {2} 名称: {3}({4})({5})", l_title, g_reg()->ctx().contains<database_info>() ? g_reg()->ctx().at<database_info>().path_ : FSys::path{":memory:"}, l_prj.p_path, l_prj.show_str(), l_prj.str(), l_prj.short_str());

    set_title(l_str);
  };
  g_reg()->ctx().at<core_sig>().project_end_open.connect(s_set_title_fun);
  g_reg()->ctx().at<core_sig>().save.connect(3, s_set_title_fun);
}

void doodle_main_app::set_title(const std::string& in_title) {
  boost::asio::post(g_io_context(), [&, in_title]() {
    auto l_str = boost::locale::conv::utf_to_utf<wchar_t>(in_title);
    SetWindowTextW(p_hwnd, l_str.c_str());
  });
}

doodle_main_app& doodle_main_app::Get() {
  return *(dynamic_cast<doodle_main_app*>(self));
}
bool doodle_main_app::valid() const {
  return this->p_hwnd != nullptr;
}
void doodle_main_app::close_windows() {
  doodle::app_command_base::Get().stop_app();
  boost::asio::post([this]() {
    ::ShowWindow(p_hwnd, SW_HIDE);
    ::DestroyWindow(doodle::doodle_main_app::Get().p_hwnd);
  });
}
void doodle_main_app::show_windows() {
  ::ShowWindow(p_hwnd, SW_SHOW);
}

doodle_main_app::~doodle_main_app() {
  // Cleanup
  ImGui_ImplDX11_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImPlot::DestroyContext();
  ImGui::DestroyContext();
  g_reg()->ctx().at<std::shared_ptr<win::d3d_device>>().reset();

  ::RevokeDragDrop(p_hwnd);
  ::DestroyWindow(p_hwnd);
  ::UnregisterClassW(p_win_class.lpszClassName, p_win_class.hInstance);
}

void doodle_main_app::load_back_end() {
  g_reg()->ctx().at<core_sig>().init_end.connect([]() {
    init_register::instance().init_run();
  });
  make_handle().emplace<gui::gui_tick>(std::make_shared<gui::short_cut>());
}

bool doodle_main_app::chick_authorization() {
  if (!app_command_base::chick_authorization()) {
    auto show_str = fmt::format("授权失败\n请见授权文件放入 {} ", core_set::get_set().get_doc() / doodle_config::token_name.data());
    ::MessageBoxExW(p_hwnd, boost::locale::conv::utf_to_utf<wchar_t>(show_str).c_str(), L"错误", MB_OK, 0);
    return false;
  }
  return true;
}
void doodle_main_app::tick_begin() {
  app_base::tick_begin();

  MSG msg;
  while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
    /// 如果时退出消息, 直接设置停止
    if (msg.message == WM_QUIT) {
      DOODLE_LOG_INFO("开始退出");
      return;
    }
  }

  // Start the Dear ImGui frame
  ImGui_ImplDX11_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();
}
void doodle_main_app::tick_end() {
  app_base::tick_end();
  // Rendering
  ImGui::Render();
  static ImVec4 clear_color             = ImVec4(0.0f, 0.0f, 0.0f, 0.00f);
  const float clear_color_with_alpha[4] = {
      clear_color.x * clear_color.w,
      clear_color.y * clear_color.w,
      clear_color.z * clear_color.w,
      clear_color.w};

  d3d_deve->g_pd3dDeviceContext->OMSetRenderTargets(1, &d3d_deve->g_mainRenderTargetView, nullptr);
  d3d_deve->g_pd3dDeviceContext->ClearRenderTargetView(d3d_deve->g_mainRenderTargetView, clear_color_with_alpha);
  ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

  // Update and Render additional Platform Windows
  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
  }

  d3d_deve->g_pSwapChain->Present(1, 0);  // Present with vsync
                                          // g_pSwapChain->Present(0, 0); // Present without vsync
}

}  // namespace doodle
