//
// Created by td_main on 2023/7/25.
//
#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include "doodle_app/gui/base/ref_base.h"
#include <doodle_app/lib_warp/imgui_warp.h>

#include "fmt/compile.h"
namespace doodle::gui::render {
namespace detail {
template <typename T>
class file_one_edit_t {
  gui_cache_name_id path_id_{};
  gui_cache_name_id add{};
  std::string path_{};
  entt::handle render_id_{};
  void init(const entt::handle& in_handle) {
    if (in_handle == render_id_) return;

    if (in_handle.any_of<T>()) {
      auto& l_path = in_handle.get<T>();
      path_        = l_path.path_.generic_string();
      render_id_   = in_handle;
    }
  }

 public:
  file_one_edit_t() = default;
  explicit file_one_edit_t(const std::string& in_gui_name)
      : path_id_(in_gui_name), add{fmt::format("添加 {}", in_gui_name)} {}

  bool render(const entt::handle& in_handle_view) {
    init(in_handle_view);
    constexpr static auto g_text{"拖拽添加文件"};

    bool on_change{false};

    if (in_handle_view.any_of<T>()) {
      if (ImGui::InputText(*path_id_, &path_)) {
        in_handle_view.patch<T>().path_ = path_;
        on_change                       = true;
      }
    } else {
      if (ImGui::Button(*add)) {
        in_handle_view.emplace<T>().path_ = path_;
        on_change                         = true;
      }
    }
    if (auto l_tip = dear::ItemTooltip{}) {
      ImGui::Text(g_text);
    }
    if (auto l_drag = dear::DragDropTarget{}) {
      if (const auto* l_data = ImGui::AcceptDragDropPayload(doodle_config::drop_imgui_id.data());
          l_data && l_data->IsDelivery()) {
        auto* l_list = static_cast<std::vector<FSys::path>*>(l_data->Data);
        if (!l_list->empty()) {
          path_ = l_list->front().generic_string();
          if (!in_handle_view.all_of<T>()) in_handle_view.emplace<T>();
          in_handle_view.patch<T>().path_ = path_;
          on_change                       = true;
        }
      }
    }

    return on_change;
  }
};

}  // namespace detail

}  // namespace doodle::gui::render
