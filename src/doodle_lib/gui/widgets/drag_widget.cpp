//
// Created by TD on 2022/1/28.
//

#include "drag_widget.h"
#include <long_task/drop_file_data.h>
#include <lib_warp/imgui_warp.h>

namespace doodle {

drag_widget::drag_widget(const drag_widget::one_sig& in_sig) {
  dear::DragDropTarget{} && [&]() {
    if (auto* l_pay = ImGui::AcceptDragDropPayload(doodle_config::drop_imgui_id.data()); l_pay) {
      auto k_list = reinterpret_cast<drop_file_data*>(l_pay->Data);

      for (auto&& i : k_list->files_) {
        in_sig(i);
        break;
      }
    }
  };
}
drag_widget::drag_widget(const drag_widget::mult_sig& in_sig) {
  dear::DragDropTarget{} && [&]() {
    if (auto* l_pay = ImGui::AcceptDragDropPayload(doodle_config::drop_imgui_id.data()); l_pay) {
      auto k_list = reinterpret_cast<drop_file_data*>(l_pay->Data);
      in_sig(k_list->files_);
    }
  };
}

drag_widget::drag_widget(const drag_widget::one_sig& in_sig, bool dir) {
  dear::DragDropTarget{} && [&]() {
    if (auto* l_pay = ImGui::AcceptDragDropPayload(doodle_config::drop_imgui_id.data()); l_pay) {
      auto k_list = reinterpret_cast<drop_file_data*>(l_pay->Data);

      decltype(k_list->files_) l_list{};

      std::copy_if(k_list->files_.begin(),
                   k_list->files_.end(), std::back_inserter(l_list),
                   [&](const FSys::path& in) {
                     return FSys::is_directory(in) == dir;
                   });

      for (auto&& i : l_list) {
        in_sig(i);
        break;
      }
    }
  };
}
drag_widget::drag_widget(const drag_widget::mult_sig& in_sig, bool dir) {
  dear::DragDropTarget{} && [&]() {
    if (auto* l_pay = ImGui::AcceptDragDropPayload(doodle_config::drop_imgui_id.data()); l_pay) {
      auto k_list = reinterpret_cast<drop_file_data*>(l_pay->Data);
      decltype(k_list->files_) l_list{};

      std::copy_if(k_list->files_.begin(),
                   k_list->files_.end(), std::back_inserter(l_list),
                   [&](const FSys::path& in) {
                     return FSys::is_directory(in) == dir;
                   });
      in_sig(l_list);
    }
  };
}
}  // namespace doodle
