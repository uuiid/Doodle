//
// Created by TD on 2022/1/5.
//

#include "file_browser.h"
#include <lib_warp/imgui_warp.h>
#include <platform/win/list_drive.h>
namespace doodle {
namespace {
class path_attr {
 public:
  explicit path_attr(const FSys::path& in_path)
      : path(in_path),
        show_name(in_path.filename().generic_string()),
        is_dir(is_directory(in_path)),
        size(is_dir ? 0 : file_size(in_path)),
        last_time(FSys::last_write_time_point(in_path)){};
  FSys::path path;
  string show_name;
  bool is_dir;
  std::size_t size;
  doodle::chrono::sys_time_pos last_time;

  operator bool() const {
    return !show_name.empty() && !path.empty();
  }
};

class filter_attr {
 public:
  explicit filter_attr(const string& in_filter)
      : show_name(in_filter),
        extension(in_filter){};
  string show_name;
  FSys::path extension;
};

}  // namespace

class file_browser::impl {
 public:
  impl() : enum_flags(), pwd(), select_path(), path_list(), filter_list(){};
  std::int32_t enum_flags;
  FSys::path pwd;
  std::vector<FSys::path> select_path;
  std::vector<path_attr> path_list;
  std::vector<filter_attr> filter_list;
};

file_browser::file_browser(flags in_flags)
    : p_i(std::make_unique<impl>()) {
  p_i->enum_flags = in_flags;
}
void file_browser::render() {
  if (imgui::Button("drive")) {
    auto k_dir = win::list_drive();
    p_i->path_list.clear();
    std::transform(k_dir.begin(), k_dir.end(), std::back_inserter(p_i->path_list),
                   [](auto& in_path) -> path_attr {
                     return path_attr{in_path};
                   });
  }
}
}  // namespace doodle
