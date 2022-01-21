//
// Created by TD on 2021/9/16.
//

#include "assets_file_widgets.h"

#include <doodle_lib/gui/action/command.h>
#include <doodle_lib/gui/action/command_files.h>
#include <doodle_lib/gui/action/command_meta.h>
#include <doodle_lib/gui/factory/attribute_factory_interface.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/metadata/metadata_cpp.h>


#include <entt/entt.hpp>

namespace doodle {
namespace details {
void table_column::render(const entt::handle& in_ptr) {
  imgui::TableNextColumn();
  frame_render(in_ptr);
}
void table_column::set_select(const entt::handle& in_) {
  table->p_current_select = in_;
  auto k_reg              = g_reg();
  command_list<comm_assets_add, comm_files_select> k_comm{};
  k_comm.set_data(in_);
  k_reg->set<widget_>(k_comm);

  table->select_change(in_);
}

bool column_id::frame_render(const entt::handle& in_ptr) {
  if (dear::Selectable(in_ptr.get<database>().get_id_str(),
                       in_ptr == table->p_current_select,
                       ImGuiSelectableFlags_SpanAllColumns)) {
    set_select(in_ptr);
  }
  return true;
}
bool column_version::frame_render(const entt::handle& in_ptr) {
  if (in_ptr.all_of<assets_file>())
    dear::Text(in_ptr.get<assets_file>().get_version_str());
  return true;
}
bool column_comment::frame_render(const entt::handle& in_ptr) {
  if (in_ptr.all_of<comment_vector>()) {
    auto& k_com = in_ptr.get<comment_vector>();
    dear::Text(k_com.get().empty() ? std::string{} : k_com.get().front().get_comment());
  } else {
    dear::Text(std::string{});
  }
  return true;
}
bool column_path::frame_render(const entt::handle& in_ptr) {
  if (in_ptr.all_of<assets_path_vector>()) {
    auto& k_path = in_ptr.get<assets_path_vector>();
    // string k_all_str{};
    string k_line_str{fmt::format("{}", k_path)};

    dear::Text(k_line_str.c_str());
    // if (!k_all_str.empty()) {
    //   imgui::SameLine();
    //   dear::HelpMarker{"(...)", k_all_str.c_str()};
    // }
  }
  return true;
}
bool column_time::frame_render(const entt::handle& in_ptr) {
  if (in_ptr.all_of<time_point_wrap>())
    dear::Text(in_ptr.get<time_point_wrap>().show_str());
  return true;
}
bool column_user::frame_render(const entt::handle& in_ptr) {
  if (in_ptr.all_of<assets_file>())
    dear::Text(in_ptr.get<assets_file>().get_user());
  return true;
}
bool column_assets::frame_render(const entt::handle& in_ptr) {
  if (in_ptr.all_of<assets>())
    dear::Text(in_ptr.get<assets>().str());

  return true;
}
bool column_season::frame_render(const entt::handle& in_ptr) {
  if (in_ptr.all_of<season>())
    dear::Text(in_ptr.get<season>().str());
  return true;
}
bool column_episodes::frame_render(const entt::handle& in_ptr) {
  if (in_ptr.all_of<episodes>())
    dear::Text(in_ptr.get<episodes>().str());
  return true;
}
bool column_shot::frame_render(const entt::handle& in_ptr) {
  if (in_ptr.all_of<shot>())
    dear::Text(in_ptr.get<shot>().str());
  return true;
}

}  // namespace details

void assets_file_widgets::set_select(const entt::handle& in) {
}

assets_file_widgets::assets_file_widgets()
    : p_current_select(),
      p_colum_list() {
}
void assets_file_widgets::init() {
  g_reg()->set<assets_file_widgets&>(*this);
  p_colum_list.emplace_back(new_object<details::column_id>(this));
  p_colum_list.emplace_back(new_object<details::column_assets>(this));
  p_colum_list.emplace_back(new_object<details::column_season>(this));
  p_colum_list.emplace_back(new_object<details::column_episodes>(this));
  p_colum_list.emplace_back(new_object<details::column_shot>(this));
  p_colum_list.emplace_back(new_object<details::column_version>(this));
  p_colum_list.emplace_back(new_object<details::column_comment>(this));
  p_colum_list.emplace_back(new_object<details::column_path>(this));
  p_colum_list.emplace_back(new_object<details::column_time>(this));
  p_colum_list.emplace_back(new_object<details::column_user>(this));
}
void assets_file_widgets::succeeded() {
}
void assets_file_widgets::failed() {
}
void assets_file_widgets::aborted() {
}
void assets_file_widgets::update(chrono::duration<chrono::system_clock::rep, chrono::system_clock::period>, void* data) {
  static auto flags{ImGuiTableFlags_::ImGuiTableFlags_SizingFixedFit |
                    ImGuiTableFlags_::ImGuiTableFlags_Resizable |
                    ImGuiTableFlags_::ImGuiTableFlags_BordersOuter |
                    ImGuiTableFlags_::ImGuiTableFlags_BordersV |
                    ImGuiTableFlags_::ImGuiTableFlags_ContextMenuInBody |
                    ImGuiTableFlags_::ImGuiTableFlags_ScrollX |
                    ImGuiTableFlags_::ImGuiTableFlags_ScrollY};
  auto k_ = g_reg()->try_ctx<handle_list>();

  if (k_ && k_->front().valid()) {
    auto& k_list = *k_;
    dear::Table{"attribute_widgets",
                boost::numeric_cast<std::int32_t>(p_colum_list.size()),
                flags} &&
        [this, &k_list]() {
          /// 添加表头
          for (auto& i : p_colum_list) {
            if (i->p_width != 0)
              imgui::TableSetupColumn(i->p_name.c_str(), 0, imgui::GetFontSize() * i->p_width);
            else
              imgui::TableSetupColumn(i->p_name.c_str());
          }

          imgui::TableHeadersRow();

          for (auto& k_h : k_list) {
            if (k_h && k_h.all_of<database>()) {
              imgui::TableNextRow();
              for (auto& l_i : p_colum_list)
                l_i->render(k_h);
              imgui::ImageButton()
            }
          }
        };
  }
}
assets_file_widgets::~assets_file_widgets() = default;

}  // namespace doodle
