//
// Created by TD on 2022/1/22.
//

#include "screenshot_widget.h"

#include <doodle_core/metadata/image_icon.h>

#include <doodle_app/gui/base/ref_base.h>
#include <doodle_app/lib_warp/imgui_warp.h>
#include <doodle_app/platform/win/windows_proc.h>

#include <doodle_lib/app/main_facet.h>
#include <doodle_lib/core/image_loader.h>

#include <platform/win/get_screenshot.h>
namespace doodle::gui {

class screenshot_widget::impl {
 public:
  std::shared_ptr<void> image_gui;
  cv::Mat image_mat;
  cv::Rect2f virtual_screen;
  cv::Rect2f mouse_rect;

  cv::Point2f mouse_begin;
  cv::Point2f mouse_end;

  bool mouse_state{};

  entt::handle handle;
  call_ptr_type callPtrType;
  gui_cache_name_id title;
};

screenshot_widget::screenshot_widget()
    : p_i(std::make_unique<impl>()) {
  p_i->title = gui_cache_name_id{std::string{name}};
}
screenshot_widget::~screenshot_widget() = default;

void screenshot_widget::set_attr() {
  ImGuiViewport* viewport = ImGui::GetMainViewport();
  auto k_rect             = win::get_system_metrics_VIRTUALSCREEN();
  auto k_size             = k_rect.size();
  ImGui::SetNextWindowSize({k_size.width, k_size.height});
  imgui::SetNextWindowPos({k_rect.x, k_rect.y});
  ImGui::SetNextWindowViewport(viewport->ID);
  p_i->image_mat      = image_loader{}.screenshot();
  p_i->image_gui      = image_loader{}.cv_to_d3d(p_i->image_mat);
  p_i->virtual_screen = win::get_system_metrics_VIRTUALSCREEN();
}
void screenshot_widget::succeeded() {
  cv::Rect2f l_rect_2_f{p_i->mouse_rect.tl() - p_i->virtual_screen.tl(), p_i->mouse_rect.br() - p_i->virtual_screen.tl()};
  image_loader{}.save(p_i->handle, p_i->image_mat, l_rect_2_f);
  boost::asio::post(
      g_io_context(),
      [l_fun = p_i->callPtrType, l_h = p_i->handle]() {
        (*l_fun)(l_h);
      }
  );
}

void screenshot_widget::render() {
  ImGui::ImageButton(p_i->image_gui.get(), {p_i->virtual_screen.size().width, p_i->virtual_screen.size().height});
  if (imgui::IsItemActive() && p_i->mouse_state) {
    p_i->mouse_end.x = imgui::GetIO().MousePos.x;
    p_i->mouse_end.y = imgui::GetIO().MousePos.y;
    p_i->mouse_rect  = {p_i->mouse_begin, p_i->mouse_end};
  }
  if (imgui::IsItemClicked() && !p_i->mouse_state) {
    p_i->mouse_begin.x = imgui::GetIO().MousePos.x;
    p_i->mouse_begin.y = imgui::GetIO().MousePos.y;
    p_i->mouse_state   = true;
  }
  if (imgui::IsItemDeactivated() && p_i->mouse_state) {
    show_attr = false;
    this->succeeded();
  }

  ImGui::GetWindowDrawList()
      ->AddRectFilled({p_i->virtual_screen.tl().x, p_i->virtual_screen.tl().y}, {p_i->virtual_screen.br().x, p_i->virtual_screen.br().y}, ImGui::ColorConvertFloat4ToU32({0.1f, 0.4f, 0.5f, 0.2f}));
  if (!p_i->mouse_rect.empty()) {
    ImGui::GetWindowDrawList()
        ->AddRectFilled({p_i->mouse_rect.tl().x, p_i->mouse_rect.tl().y}, {p_i->mouse_rect.br().x, p_i->mouse_rect.br().y}, ImGui::ColorConvertFloat4ToU32({1.0f, 1.0f, 1.0f, 0.4f}), 0.f);
    ImGui::GetWindowDrawList()
        ->AddRect({p_i->mouse_rect.tl().x, p_i->mouse_rect.tl().y}, {p_i->mouse_rect.br().x, p_i->mouse_rect.br().y}, ImGui::ColorConvertFloat4ToU32({1.0f, 0.2f, 0.2f, 0.4f}), 0.f, 5.f);
  }

  if (ImGui::IsKeyDown(ImGuiKey_Escape)) {
    imgui::CloseCurrentPopup();
  }
}
void screenshot_widget::handle_attr(const entt::handle& in) {
  p_i->handle = in;
  if (!p_i->handle.all_of<image_icon>())
    p_i->handle.emplace<image_icon>();
}
void screenshot_widget::call_save(const screenshot_widget::call_ptr_type& in) {
  p_i->callPtrType = in;
}
const std::string& screenshot_widget::title() const {
  return p_i->title.name_id;
}
std::int32_t screenshot_widget::flags() const {
  return ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
}

}  // namespace doodle::gui
