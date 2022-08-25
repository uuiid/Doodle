//
// Created by TD on 2022/5/10.
//

#include "redirection_path_info_edit.h"

#include <doodle_core/metadata/redirection_path_info.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
namespace doodle::gui {

class redirection_path_info_edit::impl {
 public:
  gui_cache<std::string> search_path_{"搜索路径"s, ""};
  gui_cache<std::string> file_name_{"文件名称"s, ""};
};
redirection_path_info_edit::redirection_path_info_edit()
    : ptr(std::make_unique<impl>()) {
}
void redirection_path_info_edit::render(const entt::handle& in) {
  if (dear::InputTextMultiline(*ptr->search_path_.gui_name, &ptr->search_path_.data))
    set_modify(true);

  if (dear::InputText(*ptr->file_name_.gui_name, &ptr->file_name_.data))
    set_modify(true);
}
void redirection_path_info_edit::init_(const entt::handle& in) {
  if (in.any_of<redirection_path_info>()) {
    auto&& l_info     = in.get<redirection_path_info>();

    ptr->file_name_   = l_info.file_name_.generic_string();
    ptr->search_path_ = fmt::to_string(fmt::join(l_info.search_path_ | ranges::views::transform([](const FSys::path& in_path) -> std::string {
                                                   return in_path.generic_string();
                                                 }),
                                                 "\n"));
  } else {
    ptr->file_name_.data.clear();
    ptr->search_path_.data.clear();
  }
}
void redirection_path_info_edit::save_(const entt::handle& in) const {
  in.emplace_or_replace<redirection_path_info>(
      ptr->search_path_.data |
          ranges::views::split('\n') |
          ranges::views::cache1 |
          ranges::views::transform([](const auto in) -> FSys::path {
            auto c = in | ranges::views::common;
            return FSys::path{c.begin(), c.end()};
          }) |
          ranges::to_vector,
      ptr->file_name_.data
  );
}

redirection_path_info_edit::~redirection_path_info_edit() = default;
}  // namespace doodle::gui
