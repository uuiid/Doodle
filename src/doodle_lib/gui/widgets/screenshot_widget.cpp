//
// Created by TD on 2022/1/22.
//

#include "screenshot_widget.h"
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/app/app.h>
namespace doodle {

class screenshot_widget::impl {
 public:
  std::vector<std::function<void()>> begen_loop;
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
  //                    dwStyle & ~WS_OVERLAPPEDWINDOW | WS_EX_LAYERED);
  //      SetWindowPos(hwnd, HWND_TOP,
  //                   desktop.left, desktop.top, desktop.right - desktop.left, desktop.bottom - desktop.top,
  //                   SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
  //      SetLayeredWindowAttributes(hwnd, RGB(0,0,0), 0, LWA_ALPHA);
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
    auto& io      = imgui::GetIO();

    //    imgui::SetNextWindowSize(io.DisplaySize);
    //    imgui::SetNextWindowPos({0, 0});
    auto hDesktop = ::GetDesktopWindow();
    RECT desktop;
    ::GetWindowRect(hDesktop, &desktop);
    ImGui::SetNextWindowSize({boost::numeric_cast<std::float_t>(desktop.right - desktop.left),
                              boost::numeric_cast<std::float_t>(desktop.bottom - desktop.top)});

    //    POINT l_point{0, 0};
    //    ::MapWindowPoints(HWND_DESKTOP, app::Get().p_hwnd, (LPPOINT)&l_point, 1);
    //    ImGui::SetNextWindowPos({boost::numeric_cast<std::float_t>(l_point.x), boost::numeric_cast<std::float_t>(l_point.y)});
    //    ImGui::SetNextWindowPos({0, 0});
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowViewport(viewport->ID);
    imgui::OpenPopup(name.data());
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
  dear::WithStyleVar{ImGuiStyleVar_WindowRounding, 0.0f} && [&]() {
    dear::PopupModal{name.data(), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize} && [&]() {
      if (imgui::Button("ok")) {
        imgui::CloseCurrentPopup();
        this->succeed();
      }
    };
  };
}
}  // namespace doodle
