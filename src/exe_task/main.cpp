/*
 * @Author: your name
 * @Date: 2020-11-16 19:05:15
 * @LastEditTime: 2020-11-30 10:53:01
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\main.cpp
 */
#include "doodle_app/gui/base/base_window.h"
#include <doodle_app/gui/get_input_dialog.h>
#include <doodle_app/gui/main_proc_handle.h>
#include <doodle_app/gui/main_status_bar.h>

#include "doodle_lib/gui/menu_bar.h"
#include "doodle_lib/gui/setting_windows.h"
#include "doodle_lib/gui/widgets/work_hour_filling.h"
#include <doodle_lib/facet/main_facet.h>
#include <doodle_lib/facet/rpc_server_facet.h>
#include <doodle_lib/gui/layout_window.h>
#include <doodle_lib/gui/menu_bar.h>

class layout : public doodle::gui::detail::layout_tick_interface {
  void init_fun() {
    using namespace doodle;
    // 我们使用ImGuiWindowFlags_NoDocking标志来使窗口不可停靠到父窗口中，因为在彼此之间有两个停靠目标会令人困惑
    ImGuiWindowFlags window_flags =  // 没有菜单 ImGuiWindowFlags_MenuBar |
        ImGuiWindowFlags_NoDocking;

    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    window_flags |=
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    // 如果使用 ImGuiDockNodeFlags_PassthruCentralNode 处理, 那么我们就不使用背景
    window_flags |= ImGuiWindowFlags_NoBackground;

    /**
     * 这里我们持续的使 对接窗口在活动的状态上, 如果 对接处于非活动状态, 那么所有的活动窗口
     * 都会丢失父窗口并脱离, 我们将无法保留停靠窗口和非停靠窗口之间的关系, 这将导致窗口被困在边缘,
     * 永远的不可见
     */
    ImGui::Begin("Doodle_DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar(3);
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
    ImGuiIO &io                               = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
      ImGuiID dockspace_id = ImGui::GetID("DOODLE_DockSpace_Root");
      ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

      static auto first_time = true;
      if (first_time) {
        first_time = false;

        ImGui::DockBuilderRemoveNode(dockspace_id);  // clear any previous layout
        ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

        // 开始将窗口停靠在创建的窗口中
        namespace menu_w = gui::config::menu_w;

        ImGui::DockBuilderDockWindow(menu_w::work_hour_filling.data(), dockspace_id);  /// \brief 主窗口的停靠
        ImGui::DockBuilderFinish(dockspace_id);
      }
    }
    ImGui::End();
  }

 public:
  layout() = default;
  bool tick() override {
    init_fun();
    static auto first_time = true;
    if (first_time) {
      first_time = false;
      doodle::gui::show_windows<doodle::gui::work_hour_filling>();
    }
    return false;
  };
};

class menu_bar : public doodle::gui::menu_bar {
 public:
 protected:
  void menu_windows() override {
    using namespace doodle;
    if (dear::MenuItem(gui::work_hour_filling::name.data())) gui::show_windows<gui::work_hour_filling>();
    if (dear::MenuItem(gui::setting_windows::name.data())) gui::show_windows<gui::setting_windows>();
  };
  void menu_tool() override{};
};

class main_facet : public doodle::main_facet {
 public:
 protected:
  void load_windows() override {
    using namespace doodle;
    g_reg()->ctx().at<gui::layout_tick>()  = std::make_shared<layout>();
    make_handle().emplace<gui::gui_tick>() = std::make_shared<menu_bar>();
    make_handle().emplace<gui::gui_tick>() = std::make_shared<gui::main_status_bar>();
  }
};
using main_app = doodle::app_command<main_facet>;

// extern "C" int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR strCmdLine, int nCmdShow) try {
extern "C" int main() try {
  main_app app{};
  try {
    return app.run();
  } catch (const std::exception &err) {
    DOODLE_LOG_WARN(boost::diagnostic_information(boost::diagnostic_information(err)));
    return 1;
  }
} catch (...) {
  return 1;
}
