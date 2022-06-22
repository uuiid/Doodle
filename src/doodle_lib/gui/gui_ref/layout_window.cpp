//
// Created by TD on 2022/4/13.
//

#include "layout_window.h"

#include <doodle_lib/gui/widgets/time_sequencer_widget.h>
namespace doodle::gui {
class layout_window::impl {
 public:
  impl() = default;
  std::map<std::string, std::unique_ptr<windows_proc>> list_windows{};

  chrono::system_clock::duration duration_{};
  void *data_{};

  std::function<void()> main_render{};
  time_sequencer_widget time_r{};

  void builder_dock() {
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

    window_flags |= ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    // 如果使用 ImGuiDockNodeFlags_PassthruCentralNode 处理, 那么我们就不使用背景
    window_flags |= ImGuiWindowFlags_NoBackground;

    /**
     * 这里我们持续的使 对接窗口在活动的状态上, 如果 对接处于非活动状态, 那么所有的活动窗口
     * 都会丢失父窗口并脱离, 我们将无法保留停靠窗口和非停靠窗口之间的关系, 这将导致窗口被困在边缘,
     * 永远的不可见
     */
    ImGui::Begin(
        "Doodle_DockSpace",
        nullptr,
        window_flags);
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

        /**
         * 分裂给节点 其中 *返回值* 和 out_id_at_dir是相同的, 而另一个是剩下的
         */
        auto dock_id_tools = dockspace_id;
        auto dock_id_filter = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.2f, nullptr, &dock_id_tools);
        auto dock_id_edit   = ImGui::DockBuilderSplitNode(dock_id_filter, ImGuiDir_Down, 0.5f, nullptr, &dock_id_filter);
        auto dock_id_main   = ImGui::DockBuilderSplitNode(dock_id_tools, ImGuiDir_Down, 0.75f, nullptr, &dock_id_tools);

        // 开始将窗口停靠在创建的窗口中
        namespace menu_w    = gui::config::menu_w;
        ImGui::DockBuilderDockWindow(menu_w::assets_filter.data(), dock_id_filter);  /// \brief 过滤器的停靠
        ImGui::DockBuilderDockWindow(menu_w::edit_.data(), dock_id_edit);            /// \brief 编辑的停靠

        ImGui::DockBuilderDockWindow(menu_w::csv_export.data(), dock_id_tools);           /// \brief 工具所在的id
        ImGui::DockBuilderDockWindow(menu_w::ue4_widget.data(), dock_id_tools);           /// \brief 工具所在的id
        ImGui::DockBuilderDockWindow(menu_w::comm_maya_tool.data(), dock_id_tools);       /// \brief 工具所在的id
        ImGui::DockBuilderDockWindow(menu_w::comm_create_video.data(), dock_id_tools);    /// \brief 工具所在的id
        ImGui::DockBuilderDockWindow(menu_w::extract_subtitles.data(), dock_id_tools);    /// \brief 工具所在的id
        ImGui::DockBuilderDockWindow(menu_w::subtitle_processing.data(), dock_id_tools);  /// \brief 工具所在的id

        ImGui::DockBuilderDockWindow(menu_w::assets_file.data(), dock_id_main);      /// \brief 主窗口的停靠
        ImGui::DockBuilderDockWindow(menu_w::long_time_tasks.data(), dock_id_main);  /// \brief 主窗口的停靠
        ImGui::DockBuilderDockWindow(menu_w::time_edit.data(), dock_id_main);  /// \brief 主窗口的停靠
        ImGui::DockBuilderFinish(dockspace_id);
      }
    }
    ImGui::End();
  }
};
layout_window::layout_window()
    : p_i(std::make_unique<impl>()) {
}

const std::string &layout_window::title() const {
  static std::string l_title{"layout_window"};
  return l_title;
}

void layout_window::init() {
  auto k_list = init_register::instance().get_derived_class<gui::window_panel>();
  for (auto &&l_item : k_list) {
    if (auto l_win = l_item.construct(); l_win) {
      auto l_win_ptr = l_win.try_cast<base_window>();
      auto l_ptr     = std::make_shared<windows_proc::warp_proc>();
      this->p_i->list_windows.emplace(
          l_win_ptr->title(),
          std::make_unique<windows_proc>(
              l_ptr,
              l_win_ptr,
              std::move(l_win)));
    };
  }
  p_i->main_render = [this]() { call_render(std::string{gui::config::menu_w::assets_file}); };
  g_reg()->ctx().emplace<layout_window &>(*this);
}

void layout_window::succeeded() {
}

void layout_window::update(const chrono::system_clock::duration &in_duration,
                           void *in_data) {
  p_i->builder_dock();

  //  const ImGuiViewport *viewport = ImGui::GetMainViewport();
  //  ImGui::SetNextWindowPos(viewport->WorkPos);
  //  ImGui::SetNextWindowSize(viewport->WorkSize);
  p_i->duration_   = in_duration;
  p_i->data_       = in_data;
  namespace menu_w = gui::config::menu_w;
  call_render(std::string{menu_w::edit_});
  call_render(std::string{menu_w::assets_filter});

  call_render(std::string{menu_w::csv_export});
  call_render(std::string{menu_w::ue4_widget});
  call_render(std::string{menu_w::comm_maya_tool});
  call_render(std::string{menu_w::comm_create_video});
  call_render(std::string{menu_w::extract_subtitles});
  call_render(std::string{menu_w::subtitle_processing});

  call_render(std::string{menu_w::assets_file});
  call_render(std::string{menu_w::long_time_tasks});

  dear::Begin{menu_w::time_edit.data()} && [&, this]() {
    p_i->time_r.tick({}, {});
  };
}

void layout_window::call_render(const std::string &in_name) {
  dear::Begin{in_name.data()} && [&, this]() {
    auto &&l_win = p_i->list_windows[in_name];
    if (l_win)
      l_win->tick(p_i->duration_,
                  p_i->data_);
  };
}

std::shared_ptr<windows_proc::warp_proc>
layout_window::render_main(const std::string &in_name) {
  if (auto &&l_i = p_i->list_windows[in_name]; l_i) {
    l_i->windows_->close();
    call_render(in_name);
    clear_windows();
  }

  auto l_show = std::make_shared<windows_proc::warp_proc>();
  auto k_list = init_register::instance().get_derived_class<gui::window_panel>();

  auto l_it   = ranges::find_if(k_list, [&](const entt::meta_type &in_item) -> bool {
    return in_item.prop("name"_hs).value() == in_name;
  });
  if (l_it != k_list.end()) {
    if (auto l_win = l_it->construct(); l_win) {
      p_i->list_windows[in_name] = std::make_unique<windows_proc>(
          l_show,
          l_win.try_cast<base_window>(),
          std::move(l_win));
    }
  }

  if (auto &&l_i = p_i->list_windows[in_name]; l_i) {
    l_i->windows_->close.connect([this]() {
      p_i->main_render = [this]() { call_render(std::string{gui::config::menu_w::assets_file}); };
    });
    l_show = l_i->warp_proc_;
  }

  p_i->main_render = [this, l_show, in_name]() {
    call_render(in_name);
  };
  return l_show;
}
void layout_window::clear_windows() {
  for (auto it = p_i->list_windows.begin(); it != p_i->list_windows.end();) {
    if (!it->second || it->second->finished() || it->second->rejected()) {
      it = p_i->list_windows.erase(it);
    } else {
      ++it;
    }
  }
}
void layout_window::main_render() {
  p_i->main_render();
}
layout_window::~layout_window() = default;
}  // namespace doodle::gui
