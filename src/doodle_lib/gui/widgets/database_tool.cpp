//
// Created by TD on 2023/12/4.
//

#include "database_tool.h"

#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/metadata.h>

#include <doodle_app/lib_warp/imgui_warp.h>

#include "doodle_lib_fwd.h"

#include <entt/entt.hpp>
namespace doodle::gui {

namespace {}

void database_tool_t::list_repeat() {
  auto l_view = g_reg()->view<database, assets_file>().each();

  auto l_uuid_map =
      ranges::views::transform(
          l_view,
          [](const decltype(*l_view.begin())& in_tuple) {
            return repeat_item{
                std::get<database&>(in_tuple).uuid(), std::get<entt::entity>(in_tuple),
                std::get<assets_file&>(in_tuple).path_attr(), std::get<assets_file&>(in_tuple).name_attr(), ""s};
          }
      ) |
      ranges::to_vector;
  std::set<uuid> l_uuid_set{};
  for (auto&& l_item : l_uuid_map) {
    if (l_uuid_set.contains(l_item.uuid_)) {
      l_item.info_ = "重复的uuid"s;
    }
    l_uuid_set.insert(l_item.uuid_);
  }
  std::set<FSys::path> l_path_set{};
  for (auto&& l_item : l_uuid_map) {
    if (l_path_set.contains(l_item.path_)) {
      l_item.info_ = "重复的路径"s;
    }
    l_path_set.insert(l_item.path_);
  }
  l_uuid_map |= ranges::actions::remove_if([](const repeat_item& in_item) { return in_item.info_.empty(); });

  l_uuid_map |= ranges::actions::stable_sort([](const repeat_item& in_l, const repeat_item& in_r) {
    return in_l.uuid_ < in_r.uuid_;
  });

  l_uuid_map |= ranges::actions::stable_sort([](const repeat_item& in_l, const repeat_item& in_r) {
    return in_l.path_ < in_r.path_;
  });
  repeat_list_ = std::move(l_uuid_map);
  repeat_list_gui_ =
      ranges::views::transform(
          repeat_list_,
          [](const repeat_item& in_item) {
            return repeat_item_gui{
                fmt::format("{}", in_item.uuid_), in_item.path_.generic_string(), in_item.name_, in_item.info_};
          }
      ) |
      ranges::to_vector;
}

bool database_tool_t::render() {
  if (ImGui::Button(*list_repeat_id)) {
    list_repeat();
  }
  //  ImGui::SameLine();
  //  if (ImGui::Button(*list_repeat_name_id)) {
  //  }

  if (auto l_table = dear::Table{*list_repeat_table_id, 5, ImGuiTableFlags_::ImGuiTableFlags_Resizable}; l_table) {
    ImGui::TableSetupColumn("uuid");
    ImGui::TableSetupColumn("路径");
    ImGui::TableSetupColumn("名称");
    ImGui::TableSetupColumn("信息");
    ImGui::TableSetupColumn("删除");
    ImGui::TableHeadersRow();
    for (auto&& l_item : repeat_list_gui_) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      dear::Text(l_item.id_);
      ImGui::TableNextColumn();
      dear::Text(l_item.path_);
      ImGui::TableNextColumn();
      dear::Text(l_item.name_);
      ImGui::TableNextColumn();
      dear::Text(l_item.info_);
      ImGui::TableNextColumn();
      if (ImGui::SmallButton(*l_item.delete_id)) {
        g_reg()->destroy(repeat_list_[&l_item - repeat_list_gui_.data()].handle_);
        boost::asio::post(g_io_context(), [this]() { list_repeat(); });
      }
    }
  }

  return is_open;
}
}  // namespace doodle::gui