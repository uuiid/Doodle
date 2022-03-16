//
// Created by TD on 2021/9/22.
//

#include "open_file_dialog.h"

#include <doodle_lib/lib_warp/imgui_warp.h>

#include <gui/widgets/file_browser.h>
#include <gui/gui_ref/ref_base.h>
namespace doodle {

;

class file_dialog::impl {
 public:
  file_browser p_file_dialog;

  /// 返回值
  select_sig p_sig;
};
file_dialog::file_dialog(const file_dialog::select_sig &in_sig,
                         const string &in_title,
                         std::int32_t in_flags,
                         const std::vector<string> &in_filters,
                         const FSys::path &in_pwd)
    : p_i(std::make_unique<impl>()) {
  p_i->p_sig = in_sig;
  p_i->p_file_dialog.set_flags(in_flags);
  p_i->p_file_dialog.set_title(in_title);
  p_i->p_file_dialog.set_filter(in_filters);
  p_i->p_file_dialog.set_pwd_path(in_pwd);
}
file_dialog::file_dialog(const file_dialog::select_sig &in_function,
                         const string &in_title)
    : file_dialog(in_function,
                  in_title,
                  (in_function.index() == 1
                       ? flags::ImGuiFileBrowserFlags_MultipleSelection
                       : flags::ImGuiFileBrowserFlags_SelectDirectory) |
                      flags::ImGuiFileBrowserFlags_SelectDirectory,
                  {},
                  FSys::current_path()) {
}
file_dialog::file_dialog(const file_dialog::select_sig &in_function,
                         const string &in_title,
                         const std::vector<string> &in_filters,
                         const FSys::path &in_pwd)
    : file_dialog(in_function,
                  in_title,
                  (in_function.index() == 1
                       ? flags::ImGuiFileBrowserFlags_MultipleSelection
                       : 0),
                  in_filters,
                  in_pwd) {
}
file_dialog::~file_dialog() = default;

void file_dialog::init() {
  p_i->p_file_dialog.open();
}
void file_dialog::update(chrono::duration<chrono::system_clock::rep, chrono::system_clock::period>, void *data) {
  p_i->p_file_dialog.render();
  if (p_i->p_file_dialog.is_ok()) {
    std::visit(entt::overloaded{
                   [&](const one_sig &in_sig) -> void {
                     *in_sig = p_i->p_file_dialog.get_select();
                     this->succeed();
                   },
                   [&](const mult_sig &in_sig) -> void {
                     *in_sig = p_i->p_file_dialog.get_selects();
                     this->succeed();
                   }},
               p_i->p_sig);
  }

  if (!p_i->p_file_dialog.is_open()) {
    this->fail();
  }
}
void file_dialog::succeeded() {
  p_i->p_file_dialog.close();
}
void file_dialog::failed() {
  p_i->p_file_dialog.close();
}
void file_dialog::aborted() {
  p_i->p_file_dialog.close();
}

class file_panel::path_info {
 public:
  explicit path_info(const FSys::path &in_path)
      : path(in_path),
        is_dir(is_directory(in_path)),
        show_name(
            fmt::format("{} {}",
                        is_directory(in_path) ? "[dir]"s : "[file]"s,
                        path.has_filename() ? path.filename().generic_string() : path.generic_string())),
        size(is_regular_file(in_path) ? file_size(in_path) : 0u),
        size_string(),
        last_time(FSys::last_write_time_point(in_path)),
        has_select(false) {
    set_size_str();
  };

  explicit path_info(const FSys::directory_iterator &in_iterator)
      : path(in_iterator->path()),
        is_dir(in_iterator->is_directory()),
        show_name(
            fmt::format("{} {}",
                        in_iterator->is_directory() ? "[dir]"s : "file"s,
                        path.has_filename() ? path.filename().generic_string() : path.generic_string())),
        size(in_iterator->is_regular_file() ? in_iterator->file_size() : 0u),
        size_string(),
        last_time(FSys::last_write_time_point(path)),
        has_select(false) {
    set_size_str();
  };

  FSys::path path;
  bool is_dir;
  string show_name;
  std::size_t size;
  string size_string;
  doodle::chrono::sys_time_pos last_time;
  bool has_select;

  void set_size_str() {
    std::int16_t i{};
    std::double_t mantissa = size;
    for (; mantissa >= 1024.; mantissa /= 1024., ++i) {
    }
    mantissa    = std::ceil(mantissa * 10.) / 10.;
    size_string = fmt::format("{} {}B", mantissa, i == 0 ? ""s : string{"BKMGTPE"[i]});
  }

  operator bool() const {
    return !show_name.empty() && !path.empty();
  }
  bool operator==(const path_info &in_rhs) const {
    return path == in_rhs.path;
  }
  bool operator!=(const path_info &in_rhs) const {
    return !(in_rhs == *this);
  }
  bool operator<(const path_info &in_rhs) const {
    return std::tie(path) < std::tie(in_rhs.path);
  }
  bool operator>(const path_info &in_rhs) const {
    return in_rhs < *this;
  }
  bool operator<=(const path_info &in_rhs) const {
    return !(in_rhs < *this);
  }
  bool operator>=(const path_info &in_rhs) const {
    return !(*this < in_rhs);
  }
};

class file_panel::impl {
  class filter_show {
   public:
    std::string show_str;
  };

 public:
  impl()
      : title_p(),
        drive_button("drive"s),
        edit_button("E"s, false),
        edit_input("##input_"),
        path_button_list(),
        path_list(),
        buffer("##info"),
        filter_list("过滤器"s, std::vector<std::string>{}),
        begin_fun_list(),
        is_ok(false),
        p_flags_(){};

  enum sort_by : std::int16_t {
    none = 0,
    name = 1,
    size = 2,
    time = 3,
  };

  std::string title_p;
  gui::gui_cache_name_id drive_button;
  gui::gui_cache<bool> edit_button;
  gui::gui_cache<std::string> edit_input;
  std::vector<gui::gui_cache<std::string, gui::gui_cache_path>> path_button_list;
  std::vector<path_info> path_list;
  gui::gui_cache<std::string> buffer;
  gui::gui_cache<std::vector<std::string>, filter_show> filter_list;

  std::vector<std::function<void()>> begin_fun_list;
  std::size_t select_index{};
  bool is_ok;
  dialog_flags p_flags_;

  file_panel::select_sig out_;
};

file_panel::default_pwd::default_pwd() : pwd(FSys::current_path()) {}

file_panel::file_panel(const dialog_args &in_args)
    : p_i(std::make_unique<impl>()) {
  p_i->title_p          = in_args.title;
  /// \brief 放置过滤器
  p_i->filter_list.data = in_args.filter;
  p_i->filter_list.data.emplace_back("*.*");
  p_i->filter_list.show_str = p_i->filter_list.data.front();
}
file_panel::file_panel(const file_panel::select_sig &out_select_ptr,
                       const string &in_title) {
}
std::string &file_panel::title() const {
  return p_i->title_p;
}
void file_panel::init() {
}
void file_panel::succeeded() {
}
void file_panel::failed() {
}
void file_panel::aborted() {
}
void file_panel::update(const chrono::duration<chrono::system_clock::rep,
                                               chrono::system_clock::period> &,
                        void *data) {
}
file_panel::dialog_args::dialog_args(file_panel::select_sig in_out_ptr)
    : out_ptr(std::move(in_out_ptr)),
      p_flags(),
      filter(),
      title("get file"),
      pwd() {
  use_default_pwd();
}
file_panel::dialog_args &file_panel::dialog_args::set_use_dir(bool use_dir) {
  p_flags[0] = use_dir;
  return *this;
}
file_panel::dialog_args &file_panel::dialog_args::create_file_module(bool use_create) {
  p_flags[1] = use_create;
  return *this;
}
file_panel::dialog_args &file_panel::dialog_args::set_title(std::string in_title) {
  title = std::move(in_title);
  return *this;
}
file_panel::dialog_args &file_panel::dialog_args::set_filter(const std::vector<std::string> &in_filters) {
  filter = in_filters;
  return *this;
}
file_panel::dialog_args &file_panel::dialog_args::add_filter(const string &in_filter) {
  filter.emplace_back(in_filter);
  return *this;
}
file_panel::dialog_args &file_panel::dialog_args::set_pwd(const FSys::path &in_pwd) {
  pwd = in_pwd;
  return *this;
}
file_panel::dialog_args &file_panel::dialog_args::use_default_pwd() {
  pwd = g_reg()->ctx_or_set<default_pwd>().pwd;
  return *this;
}
}  // namespace doodle
