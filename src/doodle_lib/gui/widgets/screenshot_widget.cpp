//
// Created by TD on 2022/1/22.
//

#include "screenshot_widget.h"
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/app/app.h>
#include <doodle_lib/core/image_loader.h>
#include <doodle_lib/platform/win/wnd_proc.h>
namespace doodle {

class screenshot_widget::impl {
 public:
  std::vector<std::function<void()>> begen_loop;
  std::shared_ptr<void> image_gui;
  cv::Rect2f virtual_screen;
};

screenshot_widget::screenshot_widget()
    : p_i(std::make_unique<impl>()) {
}
screenshot_widget::~screenshot_widget() = default;
void screenshot_widget::init() {
  //  auto hwnd                = app::Get().p_hwnd;
  //  auto dwStyle             = GetWindowLong(hwnd, GWL_STYLE);
  //  WINDOWPLACEMENT g_wpPrev = {sizeof(g_wpPrev)};
  //  if (dwStyle & WS_OVERLAPPEDWINDOW) {
  //    MONITORINFO mi = {sizeof(mi)};
  //    if (GetWindowPlacement(hwnd, &g_wpPrev) &&
  //        GetMonitorInfo(MonitorFromWindow(hwnd,
  //                                         MONITOR_DEFAULTTOPRIMARY),
  //                       &mi)) {
  //      auto hDesktop = ::GetDesktopWindow();
  //      RECT desktop;
  //      ::GetWindowRect(hDesktop, &desktop);
  //      SetWindowLong(hwnd, GWL_STYLE,
  //                    dwStyle & ~WS_OVERLAPPEDWINDOW);  // WS_EX_NOREDIRECTIONBITMAP
  //      ::SetWindowLongW(hwnd, GWL_EXSTYLE, WS_EX_LAYERED);
  //      SetWindowPos(hwnd, HWND_TOP,
  //                   desktop.left, desktop.top, desktop.right - desktop.left, desktop.bottom - desktop.top,
  //                   SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
  ////      SetLayeredWindowAttributes(hwnd, NULL, 0, LWA_ALPHA);
  //    }
  //  } else {
  //    SetWindowLong(hwnd, GWL_STYLE,
  //                  dwStyle | WS_OVERLAPPEDWINDOW);
  //    SetWindowPlacement(hwnd, &g_wpPrev);
  //    SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
  //                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
  //                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
  //  }

  p_i->begen_loop.emplace_back([&]() {
    //    auto& io      = imgui::GetIO();

    //    imgui::SetNextWindowSize(io.DisplaySize);
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    auto k_rect             = win::get_system_metrics_VIRTUALSCREEN();
    auto k_size             = k_rect.size();
    ImGui::SetNextWindowSize({k_size.width, k_size.height});
    //    ImGui::SetNextWindowSize({boost::numeric_cast<std::float_t>(GetSystemMetrics(SM_CXVIRTUALSCREEN)),
    //                              boost::numeric_cast<std::float_t>(GetSystemMetrics(SM_CYVIRTUALSCREEN))});
    imgui::SetNextWindowPos({k_rect.x, k_rect.y});

    //    POINT l_point{0, 0};
    //    ::MapWindowPoints(HWND_DESKTOP, app::Get().p_hwnd, (LPPOINT)&l_point, 1);
    //    ImGui::SetNextWindowPos({boost::numeric_cast<std::float_t>(l_point.x), boost::numeric_cast<std::float_t>(l_point.y)});
    //    ImGui::SetNextWindowPos({0, 0});

    //        ImGui::SetNextWindowPos(viewport->Pos);
    //        ImGui::SetNextWindowSize(viewport->Size);

    ImGui::SetNextWindowViewport(viewport->ID);
    p_i->image_gui      = image_loader{}.screenshot();
    p_i->virtual_screen = win::get_system_metrics_VIRTUALSCREEN();
    //    ImGui::SetNextWindowBgAlpha(0.1f);
    //    imgui::OpenPopup(name.data());
  });
}
void screenshot_widget::succeeded() {
}
void screenshot_widget::failed() {
}
void screenshot_widget::aborted() {
}
void screenshot_widget::update(chrono::duration<chrono::system_clock::rep, chrono::system_clock::period>, void* data) {
  for (auto&& fun : p_i->begen_loop) {
    fun();
  }
  p_i->begen_loop.clear();
  //  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.9f, 0.5f, 0.5f, 0.0f));
  dear::Begin{name.data(),
              nullptr,
              ImGuiWindowFlags_NoDecoration |
                  ImGuiWindowFlags_NoResize |
                  ImGuiWindowFlags_NoMove} &&
      [&]() {
        ImGui::Image(p_i->image_gui.get(), {p_i->virtual_screen.size().width,
                                            p_i->virtual_screen.size().height});
        ImGui::GetWindowDrawList()
            ->AddRectFilled({p_i->virtual_screen.x, p_i->virtual_screen.y},
                            {p_i->virtual_screen.size().width,
                             p_i->virtual_screen.size().height},
                            ImGui::ColorConvertFloat4ToU32({0.2f, 0.3f, 0.2f, 0.2f}));
        if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_Escape))) {
          imgui::CloseCurrentPopup();
          this->fail();
        }
      };
  //  ImGui::PopStyleColor();
  //  dear::WithStyleVar{ImGuiStyleVar_WindowRounding, 0.0f} && [&]() {
  //  };
}
}  // namespace doodle
