//
// Created by TD on 2023/12/4.
//
#pragma once
#include <doodle_app/gui/base/ref_base.h>
namespace doodle::gui {
class DOODLELIB_API database_tool_t {
  bool is_open{true};
  // 列出重复的文件条目
  gui_cache_name_id list_repeat_id{"列出重复的文件条目"s};
  // 列出重复名称的文件条目
  gui_cache_name_id list_repeat_name_id{"列出重复名称的文件条目"s};

  // 列出重复的文件条目

  void list_repeat();
  struct repeat_item {
    uuid uuid_{};
    entt::entity handle_{};
    FSys::path path_{};
    std::string name_{};
    std::string info_{};
  };
  struct repeat_item_gui {
    std::string id_{};
    std::string path_{};
    std::string name_{};
    std::string info_{};
    gui_cache_name_id delete_id{"删除"s};
  };

  // 重复条目
  std::vector<repeat_item> repeat_list_{};
  std::vector<repeat_item_gui> repeat_list_gui_{};
  gui_cache_name_id list_repeat_table_id{"重复条目列表"s};

 public:
  database_tool_t() = default;

  constexpr static std::string_view name{gui::config::menu_w::database_tool};
  bool render();
};
}  // namespace doodle::gui