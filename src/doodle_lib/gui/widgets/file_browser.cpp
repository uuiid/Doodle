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
        filter_list(),
        select_index(0),
        begin_fun_list(){};
  std::int32_t enum_flags;

  bool show;
  FSys::path pwd;

  std::vector<path_attr> path_list;
  std::size_t select_index{};
  std::vector<filter_attr> filter_list;

  std::vector<std::function<void()>> begin_fun_list;
};

file_browser::file_browser(flags in_flags)
    : p_i(std::make_unique<impl>()) {
  p_i->enum_flags = in_flags;
  this->scan_director(p_i->pwd);
}
void file_browser::render() {
  for (auto&& k_fun : p_i->begin_fun_list)
    k_fun();

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
  boost::remove_erase_if(k_list, [](auto in) -> bool { return in; });
  /// \brief 去除不符合过滤器的
  boost::remove_erase_if(k_list, [&](const path_attr& in) -> bool {
    return !std::any_of(p_i->filter_list.begin(), p_i->filter_list.end(), [&](const FSys::path& in_filter) -> bool {
      return in_filter == in.path.extension();
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
          if (dear::Selectable(fmt::format("{} {}", k_p.is_dir ? "[dir]"s : "[file]"s, k_p.show_name), k_p.has_select, ImGuiSelectableFlags_SpanAllColumns)) {
            if (imgui::IsMouseDoubleClicked(ImGuiMouseButton_::ImGuiMouseButton_Left)) {  /// \brief 双击函数
              p_i->begin_fun_list.emplace_back(
                  [=, in_path = k_p.path]() {
                    this->scan_director(in_path);
                  });
            } else if (imgui::IsMouseClicked(ImGuiMouseButton_::ImGuiMouseButton_Left)) {  /// \brief 单击函数
              auto& k_io = imgui::GetIO();
              if (k_io.KeyCtrl)
                k_p.has_select = !(k_p.has_select);
              else if (k_io.KeyShift) {
                std::for_each(p_i->path_list.begin() + boost::numeric_cast<std::int64_t>(p_i->select_index),
                              std::find(p_i->path_list.begin(), p_i->path_list.end(), k_p),
                              [](path_attr& in_attr) {
                                in_attr.has_select = true;
                              });
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
}  // namespace doodle
