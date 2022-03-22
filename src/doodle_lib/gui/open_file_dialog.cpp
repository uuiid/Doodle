//
// Created by TD on 2021/9/22.
//

#include "open_file_dialog.h"

#include <boost/contract.hpp>

#include <doodle_lib/lib_warp/imgui_warp.h>

#include <gui/widgets/file_browser.h>
#include <gui/gui_ref/ref_base.h>

namespace doodle {

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
        p_flags_(),
        sort_by_p(sort_by::none){};

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
  FSys::path p_pwd;
  sort_by sort_by_p;
};

file_panel::default_pwd::default_pwd()
    : default_pwd(FSys::current_path()) {}
file_panel::default_pwd::default_pwd(FSys::path in_pwd)
    : pwd(std::move(in_pwd)) {}

file_panel::file_panel(const dialog_args &in_args)
    : p_i(std::make_unique<impl>()) {
  p_i->title_p          = in_args.title;
  /// \brief 放置过滤器
  p_i->filter_list.data = in_args.filter;
  p_i->filter_list.data.emplace_back("*.*");
  p_i->filter_list.show_str = p_i->filter_list.data.front();
  /// \brief 设置标志
  p_i->p_flags_             = in_args.p_flags;
  /// \brief 设置输出
  p_i->out_                 = in_args.out_ptr;
  p_i->p_pwd                = in_args.pwd;
}

std::string &file_panel::title() const {
  return p_i->title_p;
}
void file_panel::init() {
  this->scan_director(p_i->p_pwd);
}
void file_panel::succeeded() {
  g_reg()->ctx_or_set<default_pwd>(p_i->p_pwd);
}
void file_panel::failed() {
}
void file_panel::aborted() {
}

void file_panel::update(const chrono::duration<chrono::system_clock::rep,
                                               chrono::system_clock::period> &,
                        void *data) {
  for (auto &&i : p_i->begin_fun_list)
    i();
  p_i->begin_fun_list.clear();

  /// 返回根目录按钮
  if (ImGui::Button(*p_i->drive_button)) {
    auto k_dir = win::list_drive();
    p_i->path_list.clear();
    p_i->p_pwd.clear();
    p_i->path_list = k_dir | ranges::view::transform([](auto &in_path) -> path_info {
                       return path_info{in_path};
                     }) |
                     ranges::to_vector;
  }
  /// 输入路径按钮
  ImGui::SameLine();
  if (ImGui::Button(*p_i->edit_button.gui_name)) {
    p_i->edit_button.data = true;
  }

  /// 渲染路径
  this->render_path(p_i->edit_button.data);
  /// 渲染文件列表
  this->render_list_path();
  /// 渲染缓冲区
  this->render_buffer();
  /// 完成按钮
  this->button_ok();
  /// 取消按钮
  this->button_cancel();
  /// 过滤器按钮
  this->render_filter();
}
void file_panel::scan_director(const FSys::path &in_dir) {
  // boost::contract::check l_check = boost::contract::public_function
  if (!FSys::is_directory(in_dir))
    return;

  p_i->p_pwd        = in_dir;
  p_i->select_index = 0;
  p_i->buffer.data.clear();

  p_i->path_list = ranges::make_subrange(
                       FSys::directory_iterator{in_dir},
                       FSys::directory_iterator{}) |
                   ranges::views::transform([](const auto &in) {
                     try {
                       return path_info{in};
                     } catch (const FSys::filesystem_error &err) {
                       DOODLE_LOG_ERROR(error.what());
                     }
                   }) |  /// 过滤无效
                   ranges::views::filter([](const auto &in_info) -> bool {
                     return in_info;
                   }) |  /// 过滤不符合过滤器的
                   ranges::views::filter([this](const path_info &in_info) -> bool {
                     /// 进行目录过滤
                     if (p_i->p_flags_ ^ flags_Use_dir) {
                       if (p_i->filter_list.show_str != "*.*") {
                         return in_info.is_dir ||
                                in_info.path.extension() == p_i->filter_list.show_str;
                       }
                     }

                     return true;
                   }) |
                   ranges::to_vector;
  sort_by_attr(p_i->sort_by_p);
}

void file_panel::sort_file_attr(sort_by in_sort_by) {
  p_i->path_list |=
      ranges::actions::sort(
          [&](const path_info &in_l, const path_info &in_r) -> bool {
            bool result;
            switch (in_sort_by) {
              case sort_by::name:
                result = in_l.path.filename() < in_r.path.filename();
                break;
              case sort_by::size:
                result = in_l.size < in_r.size;
                break;
              case sort_by::time:
                result = in_l.last_time < in_r.last_time;
                break;
              default:
                result = in_l < in_r;
                break;
            }
            return result;
          });
  /// \brief 将目录和文件进行分区
  ranges::stable_partition(
      p_i->path_list,
      [](const path_info &in) -> bool { return in.is_dir; });
  if (in_reverse)
    ranges::reverse(p_i->path_list);
  p_i->select_index = 0;
}

void file_panel::render_path(bool edit) {
}
void file_panel::render_list_path() {
}
void file_panel::render_buffer() {
}
void file_panel::render_filter() {
}
void file_panel::button_ok() {
}
void file_panel::button_cancel() {
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
