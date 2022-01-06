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
        is_dir(is_directory(in_path)),
        show_name(fmt::format("{} {}", is_directory(in_path) ? "[dir]"s : "file"s, path.filename().generic_string())),
        size(file_size(in_path)),
        last_time(FSys::last_write_time_point(in_path)),
        has_select(false){};

  explicit path_attr(const FSys::directory_iterator& in_iterator)
      : path(in_iterator->path()),
        is_dir(in_iterator->is_directory()),
        show_name(fmt::format("{} {}", in_iterator->is_directory() ? "[dir]"s : "file"s, path.filename().generic_string())),
        size(in_iterator->file_size()),
        last_time(FSys::last_write_time_point(path)),
        has_select(false){};

  FSys::path path;
  bool is_dir;
  string show_name;
  std::size_t size;
  doodle::chrono::sys_time_pos last_time;
  bool has_select;

  operator bool() const {
    return !show_name.empty() && !path.empty();
  }
  bool operator==(const path_attr& in_rhs) const {
    return path == in_rhs.path;
  }
  bool operator!=(const path_attr& in_rhs) const {
    return !(in_rhs == *this);
  }
  bool operator<(const path_attr& in_rhs) const {
    return std::tie(  //
               is_dir,
               last_time,
               size,
               show_name
               //
               ) < std::tie(  //
                       in_rhs.is_dir,
                       in_rhs.last_time,
                       in_rhs.size,
                       in_rhs.show_name
                       //
                   );
  }
  bool operator>(const path_attr& in_rhs) const {
    return in_rhs < *this;
  }
  bool operator<=(const path_attr& in_rhs) const {
    return !(in_rhs < *this);
  }
  bool operator>=(const path_attr& in_rhs) const {
    return !(*this < in_rhs);
  }
};

class filter_attr {
 public:
  explicit filter_attr(const string& in_filter)
      : show_name(in_filter),
        extension(in_filter){};
  explicit filter_attr(const FSys::path& in_filter)
      : show_name(in_filter.generic_string()),
        extension(in_filter){};
  string show_name;
  FSys::path extension;
};

}  // namespace

class file_browser::impl {
 public:
  explicit impl()
      : enum_flags(),
        show(false),
        pwd(FSys::current_path()),
        path_list(),
        select_index(0),
        filter_show_name("*.*"),
        current_filter_list(),
        filter_list(),
        begin_fun_list(),
        is_ok(false),
        buffer(){};
  std::int32_t enum_flags;

  bool show;
  FSys::path pwd;

  std::vector<path_attr> path_list;
  std::size_t select_index{};
  string filter_show_name;
  std::vector<filter_attr> current_filter_list;
  std::map<string, std::vector<filter_attr>> filter_list;

  std::vector<std::function<void()>> begin_fun_list;

  bool is_ok;
  string buffer;
  void set_multiple_select(path_attr& k_p) {
    auto& k_io = imgui::GetIO();
    if (k_io.KeyCtrl)
      k_p.has_select = !(k_p.has_select);
    else if (k_io.KeyShift) {
      std::for_each(path_list.begin() + boost::numeric_cast<int64_t>(select_index),
                    std::find(path_list.begin(), path_list.end(), k_p),
                    [](path_attr& in_attr) {
                      in_attr.has_select = true;
                    });
    }
    generate_buffer();
  }
  void set_one_select(path_attr& k_p) {
    std::for_each(path_list.begin(), path_list.end(), [](auto& in) {
      in.has_select = false;
    });
    k_p.has_select = true;
    generate_buffer();
  }
  void generate_buffer() {
    if (std::any_of(path_list.begin(), path_list.end(), [](const path_attr& in) -> bool { return in.has_select; })) {
      auto l_size = std::count_if(path_list.begin(), path_list.end(), [](const path_attr& in) -> bool { return in.has_select; });
      if (l_size > 1) {
        buffer = fmt::format("选中了 {} 个路径", l_size);
      } else {
        auto k_p = std::find_if(path_list.begin(), path_list.end(),[](const path_attr& in) -> bool { return in.has_select; });
        buffer = k_p->show_name;
      }
    } else {
      buffer.clear();
    }
  }
};

file_browser::file_browser(flags in_flags)
    : p_i(std::make_unique<impl>()) {
  p_i->enum_flags = in_flags;
  this->scan_director(p_i->pwd);
}
void file_browser::render() {
  for (auto&& k_fun : p_i->begin_fun_list)
    k_fun();
  p_i->begin_fun_list.clear();

  if (imgui::Button("drive")) {
    auto k_dir = win::list_drive();
    p_i->path_list.clear();
    p_i->pwd.clear();
    std::transform(k_dir.begin(), k_dir.end(), std::back_inserter(p_i->path_list),
                   [](auto& in_path) -> path_attr {
                     return path_attr{in_path};
                   });
  }

  this->render_path();
  this->render_file_list();
  this->render_buffer();
  this->render_filter();
  if (imgui::Button("ok")) {
    p_i->is_ok = true;
    p_i->show  = false;
  }
  imgui::SameLine();
  if (imgui::Button("cancel")) {
    p_i->is_ok = false;
    p_i->show  = false;
  }
}
void file_browser::render_path() {
  std::int32_t k_i{0};
  for (auto& k_p : p_i->pwd.relative_path()) {
    imgui::SameLine();
    if (imgui::Button(fmt::format("{0}##{0}_{1}", k_p.generic_string(), k_i).c_str())) {
      auto k_r = p_i->pwd.root_path();
      auto k_j{k_i};
      for (const auto& l_p : p_i->pwd.relative_path()) {
        k_r /= l_p;
        --k_j;
        if (k_j < 0) break;
      }
      p_i->begin_fun_list.emplace_back([this, in_path = k_r]() {
        p_i->pwd          = in_path;
        p_i->select_index = 0;
        this->scan_director(in_path);
      });
    }
    ++k_i;
  }
}
void file_browser::scan_director(const FSys::path& in_path) {
  decltype(p_i->path_list) k_list;
  p_i->path_list.clear();
  for (auto& k_p : FSys::directory_iterator{in_path}) {
    k_list.emplace_back(k_p);
  }
  /// \brief 去除无效的
  boost::remove_erase_if(k_list, [](auto in) -> bool { return !in; });

  /// \brief 去除不符合过滤器的
  if (p_i->enum_flags & flags_::file_browser_flags_SelectDirectory)  /// \brief 包含选中目录时直接排除所有文件
    boost::remove_erase_if(k_list, [&](const path_attr& in) {
      return !in.is_dir;
    });
  else  /// \brief 否则进行文件筛选
    boost::remove_erase_if(k_list, [&](const path_attr& in) -> bool {
      return !in.is_dir &&                         /// \brief 首先目录不参加排除 所以所有的目录直接返回 false
             !p_i->current_filter_list.empty() &&  /// \brief 然后如果当前过滤器为空,那么所有文件不参与排除 直接返回false
             !std::any_of(                         /// \brief 最后进行过滤器的排除方案 只要找到后缀名相等的 就不排除返回 false
                 p_i->current_filter_list.begin(),
                 p_i->current_filter_list.end(),
                 [&](const filter_attr& in_filter) -> bool {
                   return in_filter.extension == in.path.extension();
                 });
    });
  /// 进行排序
  std::sort(k_list.begin(), k_list.end());
}
void file_browser::render_file_list() {
  static auto table_flags{ImGuiTableFlags_::ImGuiTableFlags_SizingFixedFit |
                          ImGuiTableFlags_::ImGuiTableFlags_Resizable |
                          ImGuiTableFlags_::ImGuiTableFlags_BordersOuter |
                          ImGuiTableFlags_::ImGuiTableFlags_BordersV |
                          ImGuiTableFlags_::ImGuiTableFlags_ContextMenuInBody |
                          ImGuiTableFlags_::ImGuiTableFlags_ScrollX |
                          ImGuiTableFlags_::ImGuiTableFlags_ScrollY};
  dear::Table{
      "file list",
      3,
      table_flags} &&
      [&]() {
        for (auto& k_p : p_i->path_list) {
          imgui::TableNextRow();

          imgui::TableNextColumn();
          /// \brief 设置文件名序列
          if (dear::Selectable(k_p.show_name, k_p.has_select, ImGuiSelectableFlags_SpanAllColumns)) {
            if (imgui::IsMouseDoubleClicked(ImGuiMouseButton_::ImGuiMouseButton_Left)) {  /// \brief 双击函数
              p_i->begin_fun_list.emplace_back(
                  [=, in_path = k_p.path]() {
                    this->scan_director(in_path);
                  });
            } else if (imgui::IsMouseClicked(ImGuiMouseButton_::ImGuiMouseButton_Left)) {  /// \brief 单击函数
              /// \brief 多选时的方法
              if (p_i->enum_flags & flags_::file_browser_flags_MultipleSelection) {
                p_i->set_multiple_select(k_p);
              } else {
                p_i->set_one_select(k_p);
              }
              p_i->select_index = std::distance(p_i->path_list.begin(), std::find(p_i->path_list.begin(), p_i->path_list.end(), k_p)) - 1;
            }
          }
          /// \brief 文件大小
          dear::Text(fmt::format("{}", k_p.size));
          /// \brief 最后写入时间
          dear::Text(fmt::format("{}", k_p.last_time));
        }
      };
}

void file_browser::show() {
  p_i->show = true;
}
void file_browser::close() {
  p_i->show = false;
}

void file_browser::set_filter(const std::vector<string>& in_vector) {
  p_i->filter_list.clear();
  std::transform(in_vector.begin(), in_vector.end(), std::inserter(p_i->filter_list, p_i->filter_list.begin()),
                 [](const string& in) -> std::pair<string, std::vector<filter_attr>> {
                   return std::make_pair(in, std::vector<filter_attr>{filter_attr{in}});
                 });
  std::vector<filter_attr> k_all;
  std::transform(in_vector.begin(), in_vector.end(), std::back_inserter(k_all),
                 [](auto& in) -> filter_attr {
                   return filter_attr{in};
                 });
  p_i->filter_list.emplace(fmt::to_string(fmt::join(in_vector, ",")), k_all);
}
void file_browser::set_flags(file_browser::flags in_flags) {
  p_i->enum_flags = in_flags;
}
void file_browser::set_pwd_path(const FSys::path& in_path) {
  p_i->pwd = in_path;
  this->scan_director(p_i->pwd);
}
bool file_browser::is_ok() const {
  return p_i->is_ok;
}

FSys::path file_browser::get_select() const {
  return p_i->path_list.at(p_i->select_index).path;
}
std::vector<FSys::path> file_browser::get_selects() const {
  std::vector<FSys::path> k_r{};
  boost::copy(p_i->path_list |
                  boost::adaptors::filtered([](const path_attr& in_attr) {
                    return in_attr.has_select;
                  }) |
                  boost::adaptors::transformed([](const path_attr& in_attr) {
                    return in_attr.path;
                  }),
              std::back_inserter(k_r));
  return k_r;
}
void file_browser::render_buffer() {
  dear::Text(p_i->buffer);
}
void file_browser::render_filter() {
  imgui::SameLine();
  dear::Combo{"filter", p_i->filter_show_name.c_str()} && [&]() {
    for (auto& k_f : p_i->filter_list) {
      if (dear::Selectable(k_f.first)) {
        p_i->current_filter_list = k_f.second;
        p_i->filter_show_name    = k_f.first;
        p_i->begin_fun_list.emplace_back([this]() {
          scan_director(p_i->pwd);
        });
      }
    }
  };
}

}  // namespace doodle
