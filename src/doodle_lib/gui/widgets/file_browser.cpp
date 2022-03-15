//
// Created by TD on 2022/1/5.
//

#include "file_browser.h"

#include <magic_enum.hpp>

#include <lib_warp/imgui_warp.h>
#include <platform/win/list_drive.h>

namespace doodle {
namespace {
class path_attr {
 public:
  explicit path_attr(const FSys::path& in_path)
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

  explicit path_attr(const FSys::directory_iterator& in_iterator)
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
  bool operator==(const path_attr& in_rhs) const {
    return path == in_rhs.path;
  }
  bool operator!=(const path_attr& in_rhs) const {
    return !(in_rhs == *this);
  }
  bool operator<(const path_attr& in_rhs) const {
    return std::tie(
               path
               //
               ) < std::tie(in_rhs.path
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
        title(),
        b_open(false),
        pwd(),
        path_list(),
        select_index(0),
        filter_show_name("*.*"),
        current_filter_list(),
        filter_list(),
        begin_fun_list(),
        is_ok(false),
        buffer(){};
  enum sort_by : std::int16_t {
    none = 0,
    name = 1,
    size = 2,
    time = 3,
  };

  std::int32_t enum_flags;
  string title;

  bool b_open;
  FSys::path pwd;

  std::vector<path_attr> path_list;
  std::size_t select_index{};
  string filter_show_name;
  /**
   * @brief 当前过滤器， 为空时不进行任何过滤， 返回所有
   */
  std::vector<filter_attr> current_filter_list;
  std::map<string, std::vector<filter_attr>> filter_list;

  std::vector<std::function<void()>> begin_fun_list;

  bool is_ok;
  string buffer;
  bool input_path{false};
  std::string input_path_text{};

  void set_multiple_select(const std::size_t& l_index) {
    auto& k_io = imgui::GetIO();
    if (k_io.KeyCtrl)
      path_list[l_index].has_select = !(path_list[l_index].has_select) && this->select_match_filter(path_list[l_index]);
    else if (k_io.KeyShift) {
      std::for_each(path_list.begin() + boost::numeric_cast<int64_t>(std::min(l_index, select_index)),
                    path_list.begin() + boost::numeric_cast<int64_t>(std::max(l_index, select_index)),
                    [this](path_attr& in_attr) {
                      in_attr.has_select = this->select_match_filter(in_attr);
                    });
    } else
      this->set_one_select(path_list[l_index]);
  }
  void set_one_select(path_attr& k_p) {
    std::for_each(path_list.begin(), path_list.end(), [](auto& in) {
      in.has_select = false;
    });
    k_p.has_select = this->select_match_filter(k_p);
  }
  void generate_buffer() {
    if (std::any_of(path_list.begin(), path_list.end(), [](const path_attr& in) -> bool { return in.has_select; })) {
      auto l_size = std::count_if(path_list.begin(), path_list.end(), [](const path_attr& in) -> bool { return in.has_select; });
      if (l_size > 1) {
        buffer = fmt::format("选中了 {} 个路径", l_size);
      } else {
        auto k_p = std::find_if(path_list.begin(), path_list.end(), [](const path_attr& in) -> bool { return in.has_select; });
        buffer   = k_p->path.filename().generic_string();
      }
    } else {
      buffer.clear();
    }
  }
  /**
   * @brief首先判断传入标志, 要是目录的话就只有目录符合标志
   * 然后判断其他类型
   * @note 这个是判断选中是否符合过滤器的，而不是在扫描时判断的
   * @param in_attr 传入的路径属性
   */
  bool select_match_filter(const path_attr& in_attr) {
    if (enum_flags & flags_::file_browser_flags_SelectDirectory) {
      return in_attr.is_dir;
    } else {
      if (current_filter_list.empty())
        return true;
      return std::any_of(current_filter_list.begin(), current_filter_list.end(), [&](const filter_attr& in_filter_attr) {
        return in_filter_attr.extension == in_attr.path.extension();
      });
    }
    return false;
  }

  void sort_file_attr(const sort_by& in_sort, bool in_reverse = false) {
    switch (in_sort) {
      case sort_by::name:
        std::sort(path_list.begin(), path_list.end(),
                  [](const path_attr& in_l, const path_attr& in_r) -> bool {
                    return in_l.path.filename() < in_r.path.filename();
                  });
        break;
      case sort_by::size:
        std::sort(path_list.begin(), path_list.end(),
                  [](const path_attr& in_l, const path_attr& in_r) -> bool {
                    return in_l.size < in_r.size;
                  });
        break;
      case sort_by::time:
        std::sort(path_list.begin(), path_list.end(),
                  [](const path_attr& in_l, const path_attr& in_r) -> bool {
                    return in_l.last_time < in_r.last_time;
                  });
        break;
      default:
        std::sort(path_list.begin(), path_list.end());
        break;
    }
    /// \brief 将目录和文件进行分区
    std::stable_partition(path_list.begin(), path_list.end(), [](const path_attr& in) -> bool { return in.is_dir; });
    if (in_reverse)
      std::reverse(path_list.begin(), path_list.end());
    select_index = 0;
  };
};

file_browser::file_browser(flags in_flags)
    : p_i(std::make_unique<impl>()) {
  p_i->enum_flags = in_flags;
}
file_browser::~file_browser() = default;

void file_browser::render() {
  for (auto&& k_fun : p_i->begin_fun_list)
    k_fun();
  p_i->begin_fun_list.clear();

  //  if (p_i->b_open)

  dear::PopupModal{p_i->title.c_str(), &(p_i->b_open)} && [&]() {
    if (imgui::Button("drive")) {
      auto k_dir = win::list_drive();
      p_i->path_list.clear();
      p_i->pwd.clear();
      std::transform(k_dir.begin(), k_dir.end(), std::back_inserter(p_i->path_list),
                     [](auto& in_path) -> path_attr {
                       return path_attr{in_path};
                     });
    }
    imgui::SameLine();
    if (ImGui::Button("E")) {
      p_i->input_path_text.clear();
      p_i->input_path = true;
    }

    this->render_path();
    this->render_file_list();
    this->render_buffer();
    if (imgui::Button("ok")) {
      p_i->is_ok  = true;
      p_i->b_open = false;
      imgui::CloseCurrentPopup();
    }
    imgui::SameLine();
    if (imgui::Button("cancel")) {
      p_i->is_ok  = false;
      p_i->b_open = false;
      imgui::CloseCurrentPopup();
    }
    this->render_filter();
  };
}
void file_browser::render_path() {
  if (!p_i->input_path) {
    if (!p_i->pwd.empty()) {
      imgui::SameLine();
      if (imgui::Button(p_i->pwd.root_path().generic_string().c_str())) {
        p_i->begin_fun_list.emplace_back([this, in_path = p_i->pwd.root_path()]() {
          this->scan_director(in_path);
        });
      }
    }
  } else {
    imgui::SameLine();
    if (ImGui::InputText("路径", &p_i->input_path_text, ImGuiInputTextFlags_::ImGuiInputTextFlags_EnterReturnsTrue)) {
      p_i->pwd        = p_i->input_path_text;
      p_i->input_path = false;
      p_i->begin_fun_list.emplace_back([this]() {
        this->scan_director(p_i->pwd);
      });
    }
  }

  std::int32_t k_i{0};
  for (auto& k_p : p_i->pwd.relative_path()) {
    imgui::SameLine();
    if (imgui::Button(fmt::format("{0}##{1}", k_p.generic_string(), k_i).c_str())) {
      auto k_r = p_i->pwd.root_path();
      auto k_j{k_i};
      for (const auto& l_p : p_i->pwd.relative_path()) {
        k_r /= l_p;
        --k_j;
        if (k_j < 0) break;
      }
      p_i->begin_fun_list.emplace_back([this, in_path = k_r]() {
        this->scan_director(in_path);
      });
    }
    ++k_i;
  }
}
void file_browser::scan_director(const FSys::path& in_path) {
  if (!is_directory(in_path))
    return;

  static FSys::path l_path_old{};
  FSys::path l_path{in_path};
  if (l_path.empty())
    l_path = l_path_old;
  if (l_path.empty())
    return;

  /// \brief 清除数据
  p_i->pwd = l_path;
  p_i->path_list.clear();
  p_i->select_index = 0;
  p_i->buffer.clear();

  decltype(p_i->path_list) k_list;
  for (auto& k_p : FSys::directory_iterator{l_path}) {
    try {
      k_list.emplace_back(k_p);
    } catch (const FSys::filesystem_error& error) {
      DOODLE_LOG_ERROR(error.what());
    }
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
  p_i->path_list = std::move(k_list);
  p_i->sort_file_attr(p_i->none);
}
void file_browser::render_file_list() {
  static auto table_flags{
      ImGuiTableFlags_::ImGuiTableFlags_SizingFixedFit |
      ImGuiTableFlags_::ImGuiTableFlags_Resizable |
      ImGuiTableFlags_::ImGuiTableFlags_BordersOuter |
      ImGuiTableFlags_::ImGuiTableFlags_ScrollX |
      ImGuiTableFlags_::ImGuiTableFlags_ScrollY |
      ImGuiTableFlags_::ImGuiTableFlags_Sortable};
  dear::Table{
      "file list",
      3,
      table_flags,
      ImVec2(0.0f, -ImGui::GetTextLineHeightWithSpacing() * 3)} &&
      [&]() {
        /// \brief 设置题头元素
        imgui::TableSetupColumn("file name", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed, 0.0f, p_i->name);
        imgui::TableSetupColumn("size", ImGuiTableColumnFlags_WidthFixed, 0.0f, p_i->size);
        imgui::TableSetupColumn("last write time", ImGuiTableColumnFlags_WidthFixed, 0.0f, p_i->time);
        imgui::TableHeadersRow();

        /// \brief 这里进行排序
        if (auto* l_sorts_specs = dear::TableGetSortSpecs()) {
          if (l_sorts_specs->SpecsDirty) {
            p_i->sort_file_attr(
                magic_enum::enum_cast<impl::sort_by>(
                    boost::numeric_cast<impl::sort_by>(l_sorts_specs->Specs[0].ColumnUserID))
                    .value(),
                l_sorts_specs->Specs[0].SortDirection == ImGuiSortDirection_Ascending);
          }
          l_sorts_specs->SpecsDirty = false;
        }

        for (auto& k_p : p_i->path_list) {
          imgui::TableNextRow();
          /// \brief 设置文件名序列
          imgui::TableNextColumn();
          if (dear::Selectable(k_p.show_name, k_p.has_select, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick)) {
            auto l_index = std::distance(p_i->path_list.begin(), std::find(p_i->path_list.begin(), p_i->path_list.end(), k_p));

            if (imgui::IsMouseDoubleClicked(ImGuiMouseButton_::ImGuiMouseButton_Left) && k_p.is_dir) {  /// \brief 双击函数
              p_i->begin_fun_list.emplace_back(
                  [=, in_path = k_p.path]() {
                    this->scan_director(in_path);
                  });
            } else /*(imgui::IsMouseClicked(ImGuiMouseButton_::ImGuiMouseButton_Left))*/ {  /// \brief 单击函数
              /// \brief 多选时的方法
              if (p_i->enum_flags & flags_::file_browser_flags_MultipleSelection) {
                p_i->set_multiple_select(l_index);
              } else {
                p_i->set_one_select(k_p);
              }
              p_i->generate_buffer();
              p_i->select_index = l_index;
            }
          }
          /// \brief 文件大小
          imgui::TableNextColumn();
          dear::Text(k_p.size_string);
          /// \brief 最后写入时间
          imgui::TableNextColumn();
          dear::Text(fmt::format("{}", k_p.last_time));
        }
      };
}

void file_browser::open() {
  p_i->b_open = true;
  render();
  p_i->begin_fun_list.emplace_back([this]() {
    imgui::OpenPopup(p_i->title.c_str());
    imgui::SetNextWindowSize({640, 360});
  });
}
void file_browser::close() {
  p_i->b_open = false;
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
  p_i->filter_list.emplace("*.*"s, std::vector<filter_attr>{});
}
void file_browser::set_flags(file_browser::flags in_flags) {
  p_i->enum_flags = in_flags;
}
void file_browser::set_pwd_path(const FSys::path& in_path) {
  this->scan_director(in_path);
}
bool file_browser::is_ok() const {
  return p_i->is_ok;
}

FSys::path file_browser::get_select() const {
  if (std::any_of(p_i->path_list.begin(), p_i->path_list.end(), [](const path_attr& in) {
        return in.has_select;
      }))
    return p_i->path_list.at(p_i->select_index)
        .path;
  else
    return p_i->pwd;
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
  if (!k_r.empty())
    return k_r;
  else
    return {p_i->pwd};
}
void file_browser::render_buffer() {
  imgui::InputText("file", &(p_i->buffer));
}
void file_browser::render_filter() {
  imgui::SameLine();
  dear::Combo{"##filter", p_i->filter_show_name.c_str()} && [&]() {
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
void file_browser::set_title(const string& in_string) {
  p_i->title = fmt::format("{0}##{1}", in_string, fmt::ptr(this));
}
bool file_browser::is_open() const {
  return p_i->b_open;
}

}  // namespace doodle
