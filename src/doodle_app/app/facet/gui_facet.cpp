//
// Created by TD on 2022/9/30.
//

#include "gui_facet.h"
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
#include <doodle_app/app/short_cut.h>
#include <doodle_app/lib_warp/icon_font_macro.h>
#include <boost/locale.hpp>

#include <doodle_core/core/app_base.h>
#include <doodle_core/core/program_info.h>
#include <doodle_core/gui_template/show_windows.h>
#include <doodle_core/thread_pool/process_pool.h>

#include <doodle_app/lib_warp/imgui_warp.h>

#include <implot.h>
#include <implot_internal.h>
// Helper functions
#include <d3d11.h>
#include <tchar.h>
// 启用窗口拖拽导入头文件
#include <shellapi.h>

#include <imgui.h>

namespace doodle::facet {
class gui_facet::impl {
 public:
  /// \brief 初始化 com
  [[maybe_unused]] win::ole_guard _guard{};
  win::wnd_handle parent{};
  std::int32_t show_enum{};
  win::d3d_device_ptr d3d_attr;

 public:
};

const std::string& gui_facet::name() const noexcept {
  return name_attr;
}
void gui_facet::operator()() {
  post_constructor();
  make_handle().emplace<::doodle::gui::gui_tick>(std::make_shared<gui::short_cut>());
  timer_.cancel();
  static std::function<void(const boost::system::error_code& in_code)> s_fun{};
  s_fun = [&](const boost::system::error_code& in_code) {
    if (in_code == boost::asio::error::operation_aborted)
      return;
    if (g_reg()->ctx().at<::doodle::program_info>().stop_attr())
      return;
    this->loop_one();  /// \brief 各种
    this->tick_begin();
    this->tick();      /// 渲染
    this->tick_end();  /// 渲染结束
    if (!g_reg()->ctx().at<::doodle::program_info>().stop_attr()) {
      timer_.expires_after(doodle::chrono::seconds{1} / 60);
      timer_.async_wait(s_fun);
    }
  };

  timer_.expires_after(doodle::chrono::seconds{1} / 60);
  timer_.async_wait(s_fun);
}
void gui_facet::deconstruction() {
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
gui_facet::gui_facet()
    : name_attr("gui_windows"),
      timer_(g_io_context()),
      p_i(std::make_unique<impl>()) {
  g_reg()->ctx().emplace<gui::main_proc_handle>();
  g_reg()->ctx().emplace<gui::detail::layout_tick>();
}

void gui_facet::loop_one() {
  static decltype(chrono::system_clock::now()) s_now{chrono::system_clock::now()};
  decltype(chrono::system_clock::now()) l_now{chrono::system_clock::now()};
  g_bounded_pool().update(l_now - s_now, nullptr);
  s_now = l_now;
}
void gui_facet::tick() {
  auto l_lay =
      g_reg()->ctx().find<gui::detail::layout_tick>();
  if (l_lay && *l_lay) (*l_lay)->tick();

  std::vector<entt::entity> delete_entt{};
  for (auto&& [l_e, l_render] : g_reg()->view<gui::detail::windows_tick>().each()) {
    if (l_render->tick()) {
      delete_entt.emplace_back(l_e);
    }
  }
  for (auto&& [l_e, l_render] : g_reg()->view<gui::detail::windows_render>().each()) {
    if (l_render->tick()) {
      delete_entt.emplace_back(l_e);
    }
  }
  delete_entt |= ranges::action::remove_if([](const entt::entity in) -> bool {
    return !g_reg()->valid(in);
  });
  g_reg()->destroy(delete_entt.begin(), delete_entt.end());
}
void gui_facet::tick_begin() {
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
void gui_facet::tick_end() {
  // Rendering
  ImGui::Render();
  static ImVec4 clear_color             = ImVec4(0.0f, 0.0f, 0.0f, 0.00f);
  const float clear_color_with_alpha[4] = {
      clear_color.x * clear_color.w,
      clear_color.y * clear_color.w,
      clear_color.z * clear_color.w,
      clear_color.w};

  p_i->d3d_attr->g_pd3dDeviceContext->OMSetRenderTargets(1, &p_i->d3d_attr->g_mainRenderTargetView, nullptr);
  p_i->d3d_attr->g_pd3dDeviceContext->ClearRenderTargetView(p_i->d3d_attr->g_mainRenderTargetView, clear_color_with_alpha);
  ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

  // Update and Render additional Platform Windows
  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
  }

  p_i->d3d_attr->g_pSwapChain->Present(1, 0);  // Present with vsync
                                               // g_pSwapChain->Present(0, 0); // Present without vsync
}
void gui_facet::post_constructor() {
  auto l_instance = g_reg()->ctx().at<program_info>().handle_attr();

  p_win_class =
      {sizeof(WNDCLASSEX),
       CS_CLASSDC,
       win::WndProc,
       0L,
       0L,
       l_instance,
       nullptr, nullptr, nullptr, nullptr,
       _T("doodle"),
       nullptr};
  p_win_class.hIconSm = (HICON)LoadImageW(
      l_instance,
      MAKEINTRESOURCEW(1),
      IMAGE_ICON, 0, 0, LR_DEFAULTSIZE
  );
  p_win_class.hIcon = p_win_class.hIconSm;

  // Create application window
  // ImGui_ImplWin32_EnableDpiAwareness();
  ::RegisterClassExW(&p_win_class);
  auto l_str = boost::locale::conv::utf_to_utf<wchar_t>(g_reg()->ctx().at<program_info>().title_attr());
  p_hwnd     = ::CreateWindowExW(
      0L,
      p_win_class.lpszClassName,
      l_str.c_str(),
      WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
      g_reg()->ctx().at<program_info>().parent_windows_attr(),
      nullptr,
      p_win_class.hInstance,
      nullptr
  );

  // Initialize Direct3D
  p_i->d3d_attr = g_reg()->ctx().emplace<std::shared_ptr<win::d3d_device>>(std::make_shared<win::d3d_device>(p_hwnd));

  // Show the window
  ::ShowWindow(p_hwnd, SW_SHOWDEFAULT);
  ::UpdateWindow(p_hwnd);

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
  ImGui_ImplDX11_Init(p_i->d3d_attr->g_pd3dDevice, p_i->d3d_attr->g_pd3dDeviceContext);

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
    // io.Fonts->AddFontFromFileTTF(FONT_ICON_F
    // ILE_NAME_FAS, 16.0f, &icons_config, icons_ranges);
    io.Fonts->AddFontFromMemoryTTF((void*)l_font.begin(), boost::numeric_cast<std::int32_t>(l_font.size()), 16.0f, &icons_config, icons_ranges);
  }

  g_reg()->ctx().at<core_sig>().init_end.connect([this]() {
    auto l_op = g_reg()->ctx().at<program_options_ptr>();
    /// 在这里我们加载项目
    ::doodle::app_base::Get().load_project(
        l_op &&
                !l_op->p_project_path.empty()
            ? l_op->p_project_path
            : core_set::get_set().project_root[0]
    );
    boost::asio::post(g_io_context(), [this]() { this->load_windows(); });
  });

  DOODLE_CHICK(::IsWindowUnicode(p_hwnd), doodle_error{"错误的窗口"});

  static std::function<void()> s_set_title_fun{};
  s_set_title_fun = [this]() {
    auto& l_prj  = g_reg()->ctx().at<project>();
    auto l_title = boost::locale::conv::utf_to_utf<char>(g_reg()->ctx().at<program_info>().title_attr());
    auto l_str   = fmt::format(
        "{0} 文件 {1} 项目路径 {2} 名称: {3}({4})({5})",
        l_title,
        g_reg()->ctx().at<database_info>().path_,
        l_prj.p_path,
        l_prj.show_str(),
        l_prj.str(),
        l_prj.short_str()
    );
    set_title(l_str);
  };
  g_reg()->ctx().at<core_sig>().project_end_open.connect(s_set_title_fun);
  g_reg()->ctx().at<core_sig>().save.connect(3, s_set_title_fun);
}
void gui_facet::close_windows() {
  boost::asio::post(g_io_context(), [l_hwnd = p_hwnd, this]() {
    ::ShowWindow(l_hwnd, SW_HIDE);
    ::DestroyWindow(l_hwnd);
    doodle::app_base::Get().stop_app();
  });
}
void gui_facet::show_windows() const {
  ::ShowWindow(p_hwnd, SW_SHOW);
}
void gui_facet::set_title(const std::string& in_title) const {
  boost::asio::post(g_io_context(), [&, in_title]() {
    auto l_str = boost::locale::conv::utf_to_utf<wchar_t>(in_title);
    SetWindowTextW(p_hwnd, l_str.c_str());
  });
}
gui_facet::~gui_facet() = default;
}  // namespace doodle::facet
