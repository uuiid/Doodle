//
// Created by td_main on 2023/11/13.
//

#pragma once

#include <doodle_core/metadata/file_association.h>

#include <doodle_app/gui/base/ref_base.h>
namespace doodle::gui::render {
class file_association_edit_t {
  gui_cache_name_id path_id_{};
  gui_cache_name_id add{};
  std::string name_{};
  std::string tool_tip_{"拖拽两个文件到输入框中, 将会对两个文嘉进行关联"};
  std::string maya_file_{};
  std::string maya_rig_file_{};
  std::string ue_file_{};
  std::string ue_preset_file_{};
  gui_cache_name_id maya_file_id{"关联maya文件"};
  gui_cache_name_id maya_rig_file_id{"关联maya骨骼文件"};
  gui_cache_name_id ue_file_id{"关联ue文件"};
  gui_cache_name_id ue_preset_file_id{"关联ue预调文件"};
  entt::handle render_id_{};
  // 文件关联句柄
  entt::handle file_association_handle_{};

  void init(const entt::handle& in_handle);
  void create_file_association();
  entt::handle create_file_association_handle(const entt::handle& in_handle = {});

  entt::handle get_drop_handle();

 public:
  file_association_edit_t() = default;
  explicit file_association_edit_t(const std::string& in_gui_name)
      : path_id_(in_gui_name), add{fmt::format("添加 {}", in_gui_name)} {}

  bool render(const entt::handle& in_handle_view);
};
}  // namespace doodle::gui::render