//
// Created by TD on 2022/1/18.
//

#include "app.h"
#include <platform/win/wnd_proc.h>
#include <doodle_core/core/core_set.h>
#include <doodle_core/thread_pool/process_pool.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/gui/main_menu_bar.h>
#include <doodle_lib/gui/main_status_bar.h>
#include <doodle_lib/gui/gui_ref/layout_window.h>
#include <doodle_core/platform/win/drop_manager.h>
#include <doodle_lib/long_task/short_cut.h>
#include <doodle_lib/core/image_loader.h>
#include <doodle_core/core/core_sig.h>
#include <doodle_core/gui_template/gui_process.h>
#include <doodle_core/core/init_register.h>
#include <doodle_lib/gui/main_proc_handle.h>
#include <doodle_lib/gui/get_input_dialog.h>

#include <doodle_lib/core/program_options.h>
#include <doodle_lib/lib_warp/icon_font_macro.h>

#include <implot.h>
#include <implot_internal.h>

#include <gui/strand_gui.h>

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
class app::impl {
 public:
  /// \brief 初始化 com
  [[maybe_unused]] win::ole_guard _guard;

 public:
};

app::app(const win::wnd_instance& in_instance, const win::wnd_handle& in_parent)
    : app_command_base(in_instance ? in_instance : (::GetModuleHandleW(nullptr))),
      p_hwnd(),
      p_win_class(),
      d3d_deve(),
      p_show_err(false),
      d3dDevice(nullptr),
      d3dDeviceContext(nullptr),
      p_i(std::make_unique<impl>()) {
}

void app::post_constructor() {
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
  p_hwnd   = ::CreateWindowExW(0L,
                               p_win_class.lpszClassName,
                               p_title.c_str(),
                               WS_OVERLAPPEDWINDOW,
                               100, 100, 1280, 800,
                               in_parent,
                               nullptr,
                               p_win_class.hInstance,
                               nullptr);

  // Initialize Direct3D
  d3d_deve = std::make_shared<win::d3d_device>(p_hwnd);

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
  ImGui_ImplWin32_Init(p_hwnd);
  ImGui_ImplDX11_Init(d3d_deve->g_pd3dDevice, d3d_deve->g_pd3dDeviceContext);
  d3dDevice        = d3d_deve->g_pd3dDevice;
  d3dDeviceContext = d3d_deve->g_pd3dDeviceContext;
  /// 启用文件拖拽
  DragAcceptFiles(p_hwnd, true);
  /// \brief 注册拖放对象
  auto k_r = RegisterDragDrop(p_hwnd, new win::drop_manager{});
  chick_true<doodle_error>(k_r == S_OK, "无法注册拖拽com");

  //  ::ShowWindow(p_impl->p_hwnd, SW_HIDE);
  //  HMONITOR hmon  = MonitorFromWindow(p_impl->p_hwnd,
  //                                     MONITOR_DEFAULTTONEAREST);
  //  MONITORINFO mi = {sizeof(mi)};
  //  auto k_r       = GetMonitorInfo(hmon, &mi);
  //  chick_true<doodle_error>(k_r != 0,  "无法设置全屏");
  //  SetWindowPos(p_impl->p_hwnd, nullptr, mi.rcMonitor.left,
  //               mi.rcMonitor.top,
  //               mi.rcMonitor.right - mi.rcMonitor.left,
  //               mi.rcMonitor.bottom - mi.rcMonitor.top,
  //               SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
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
  auto imgui_file_path = core_set::getSet().get_cache_root("imgui") / "imgui.ini";
  static std::string _l_p{imgui_file_path.generic_string()};
  io.IniFilename = _l_p.c_str();

  //  ImGuiIO& io = ImGui::GetIO();
  //  io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\simkai.ttf)", 16.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
  {
    // io.Fonts->AddFontDefault();
    io.Fonts->AddFontFromFileTTF(doodle_config::font_default.data(), 16.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
    auto l_font                         = cmrc::DoodleLibResource::get_filesystem().open("resource/fa-solid-900.ttf");
    static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
    ImFontConfig icons_config;
    icons_config.MergeMode            = true;
    icons_config.PixelSnapH           = true;
    icons_config.FontDataOwnedByAtlas = false;
    // io.Fonts->AddFontFromFileTTF(FONT_ICON_FILE_NAME_FAS, 16.0f, &icons_config, icons_ranges);
    io.Fonts->AddFontFromMemoryTTF((void*)l_font.begin(), l_font.size(), 16.0f, &icons_config, icons_ranges);
  }

  g_reg()->ctx().at<core_sig>().init_end.connect([this]() {
    /// 在这里我们加载项目
    load_project(app::Get().options_ ? app::Get().options_->p_project_path : FSys::path{});
    boost::asio::post(g_io_context(), [this]() { this->load_windows(); });
  });

  chick_true<doodle_error>(::IsWindowUnicode(p_hwnd), "错误的窗口");
  /// \brief 设置窗口句柄处理
  gui::main_proc_handle::get().win_close = [this]() {
    auto l_quit = std::make_shared<bool>(false);
    boost::asio::post(
        make_process_adapter<gui::input::get_bool_dialog>(
            strand_gui{g_io_context()},
            l_quit)
            .next([l_quit, this]() {
              if (*l_quit)
                this->close_windows();
            }));
  };
  gui::main_proc_handle::get().win_destroy = [=]() {
    ::DestroyWindow(p_hwnd);
  };

  static std::function<void()> s_set_title_fun{};
  s_set_title_fun = [this]() {
    auto& l_prj  = g_reg()->ctx().at<project>();
    auto l_title = conv::utf_to_utf<char>(p_title);
    auto l_str   = fmt::format("{0} 文件 {1} 项目路径 {2} 名称: {3}({4})({5})",
                               l_title,
                             g_reg()->ctx().contains<database_info>() ? g_reg()->ctx().at<database_info>().path_ : FSys::path{":memory:"},
                               l_prj.p_path,
                               l_prj.show_str(),
                               l_prj.str(),
                               l_prj.short_str());

    set_title(l_str);
  };
  g_reg()->ctx().at<core_sig>().project_end_open.connect(s_set_title_fun);
  g_reg()->ctx().at<core_sig>().save.connect(3, s_set_title_fun);
}

void app::set_title(const std::string& in_title) {
  boost::asio::post(g_io_context(),
                    [&, in_title]() {
                      auto l_str = conv::utf_to_utf<wchar_t>(in_title);
                      SetWindowTextW(p_hwnd,
                                     l_str.c_str());
                    });
}

app& app::Get() {
  return *(dynamic_cast<app*>(self));
}
bool app::valid() const {
  return this->p_hwnd != nullptr;
}
void app::close_windows() {
  doodle::app_base::Get().stop_app();
  ::ShowWindow(p_hwnd, SW_HIDE);
  ::DestroyWindow(doodle::app::Get().p_hwnd);
}
void app::show_windows() {
  ::ShowWindow(p_hwnd, SW_SHOW);
}
void app::load_windows() {
  boost::asio::post(
      make_process_adapter<gui::layout_window>(strand_gui{g_io_context()}));
  boost::asio::post(
      make_process_adapter<main_menu_bar>(strand_gui{g_io_context()}));
  boost::asio::post(
      make_process_adapter<main_status_bar>(strand_gui{g_io_context()}));
}
app::~app() {
  // Cleanup
  ImGui_ImplDX11_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImPlot::DestroyContext();
  ImGui::DestroyContext();

  ::RevokeDragDrop(p_hwnd);
  ::DestroyWindow(p_hwnd);
  ::UnregisterClass(p_win_class.lpszClassName, p_win_class.hInstance);
}

void app::load_back_end() {
  g_reg()->ctx().at<core_sig>().init_end.connect([]() {
    init_register::instance().init_run();
  });

  boost::asio::post(make_process_adapter<short_cut>(strand_gui{g_io_context()}));
}

bool app::set_parent(win::wnd_handle in_parent) {
  //  ::SetWindowLongW(p_hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_CHILD);
  return ::SetParent(p_hwnd, in_parent) != nullptr;
}
bool app::chick_authorization() {
  if (!app_command_base::chick_authorization()) {
    auto show_str = fmt::format(L"授权失败\n请见授权文件放入 {} ",
                                core_set::getSet().get_doc() /
                                    doodle_config::token_name.data());
    ::MessageBoxExW(p_hwnd, show_str.c_str(), L"错误", MB_OK, 0);
    return false;
  }
  return true;
}

}  // namespace doodle
