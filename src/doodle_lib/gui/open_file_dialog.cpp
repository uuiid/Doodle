//
// Created by TD on 2021/9/22.
//

#include "open_file_dialog.h"

#include <boost/contract.hpp>
#include <magic_enum.hpp>
#include <doodle_lib/lib_warp/imgui_warp.h>

#include <gui/gui_ref/ref_base.h>
#include <doodle_core/platform/win/list_drive.h>

namespace doodle::gui {

class file_panel::path_info {
 public:
  path_info() : path(),
                is_dir(),
                show_name(),
                size(),
                size_string(),
                last_time(),
                has_select(){};

  path_info &init(const FSys::path &in_path) {
    path   = in_path;
    is_dir = is_directory(in_path);

    show_name =
        fmt::format(
            "{} {}",
            is_directory(in_path) ? "[dir]"s : "[file]"s,
            path.has_filename() ? path.filename().generic_string() : path.generic_string()
        );
    size       = is_regular_file(in_path) ? file_size(in_path) : 0u;
    last_time  = FSys::last_write_time_point(in_path);
    has_select = false;
    set_size_str();
    return *this;
  }

  FSys::path path;
  bool is_dir;
  std::string show_name;
  std::size_t size;
  std::string size_string;
  doodle::chrono::sys_time_pos last_time;
  bool has_select;

  void set_size_str() {
    std::int16_t i{};
    std::double_t mantissa = size;
    for (; mantissa >= 1024.; mantissa /= 1024., ++i) {
    }
    mantissa    = std::ceil(mantissa * 10.) / 10.;
    size_string = fmt::format("{} {}B", mantissa, i == 0 ? ""s : std::string{"BKMGTPE"[i]});
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
        buffer("##info", ""s),
        filter_list("过滤器"s, std::vector<std::string>{}),
        begin_fun_list(),
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
  dialog_flags p_flags_;

  file_panel::select_sig out_;
  FSys::path p_pwd;
  sort_by sort_by_p;
  mult_fun call_fun;
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
  g_reg()->ctx().erase<default_pwd>();
  g_reg()->ctx().emplace<default_pwd>(p_i->p_pwd);

  boost::asio::post(
      g_io_context(),
      [l_files = get_selects(), l_fun = std::move(p_i->call_fun)]() {
        l_fun(l_files);
      }
  );
}

void file_panel::render() {
  for (auto &&i : p_i->begin_fun_list)
    i();
  p_i->begin_fun_list.clear();

  /// 返回根目录按钮
  if (ImGui::Button(*p_i->drive_button)) {
    auto k_dir = win::list_drive();
    p_i->path_list.clear();
    p_i->p_pwd.clear();
    p_i->path_list = k_dir | ranges::view::transform([](auto &in_path) -> path_info {
                       return path_info{}.init(in_path);
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
  imgui::SameLine();
  this->button_cancel();
  /// 过滤器按钮
  imgui::SameLine();
  this->render_filter();
}
void file_panel::scan_director(const FSys::path &in_dir) {
  // boost::contract::check l_check = boost::contract::public_function
  if (!FSys::is_directory(in_dir))
    return;

  p_i->p_pwd        = in_dir.generic_string();
  p_i->select_index = 0;
  p_i->buffer.data.clear();
  p_i->edit_input.data = in_dir.generic_string();

  p_i->path_list       = ranges::make_subrange(
                       FSys::directory_iterator{in_dir},
                       FSys::directory_iterator{}
                   ) |
                   ranges::views::transform([](const auto &in) {
                     path_info l_info{};
                     try {
                       l_info.init(in.path());
                     } catch (const FSys::filesystem_error &err) {
                       DOODLE_LOG_ERROR(boost::diagnostic_information(err));
                     }
                     return l_info;
                   }) |  /// 过滤无效
                   ranges::views::filter([](const auto &in_info) -> bool {
                     return in_info;
                   }) |  /// 过滤不符合过滤器的
                   ranges::views::filter([this](const path_info &in_info) -> bool {
                     /// 进行目录过滤
                     if (!p_i->p_flags_[0]) {
                       if (p_i->filter_list.show_str != "*.*") {
                         return in_info.is_dir ||
                                in_info.path.extension() == p_i->filter_list.show_str;
                       }
                     } else {
                       return in_info.is_dir;
                     }
                     return true;
                   }) |
                   ranges::to_vector;
  sort_file_attr(p_i->sort_by_p);
}

void file_panel::sort_file_attr(sort_by in_sort_by, bool in_reverse) {
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
          }
      );
  /// \brief 将目录和文件进行分区
  ranges::stable_partition(
      p_i->path_list,
      [](const path_info &in) -> bool { return in.is_dir; }
  );
  if (in_reverse)
    ranges::reverse(p_i->path_list);
  p_i->select_index = 0;
}

void file_panel::render_path(bool edit) {
  if (!edit) {
    if (!p_i->p_pwd.empty()) {
      imgui::SameLine();
      if (imgui::Button(p_i->p_pwd.root_path().generic_string().c_str())) {
        p_i->begin_fun_list.emplace_back([this, in_path = p_i->p_pwd.root_path()]() {
          this->scan_director(in_path);
        });
      }
    }

    std::int32_t k_i{0};
    for (auto &k_p : p_i->p_pwd.relative_path()) {
      auto l_str = k_p.generic_string();
      if (l_str.empty())
        break;
      imgui::SameLine();
      if (imgui::Button(fmt::format("{0}##{1}", k_p.generic_string(), k_i).c_str())) {
        auto k_r = p_i->p_pwd.root_path();
        auto k_j{k_i};
        for (const auto &l_p : p_i->p_pwd.relative_path()) {
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
  } else {
    imgui::SameLine();
    if (ImGui::InputText(*p_i->edit_input.gui_name, &p_i->edit_input.data, ImGuiInputTextFlags_::ImGuiInputTextFlags_EnterReturnsTrue)) {
      p_i->edit_button.data = false;
      p_i->begin_fun_list.emplace_back([this]() {
        this->scan_director(p_i->edit_input.data);
      });
    }
  }
}
void file_panel::render_list_path() {
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
        ImGui::TableSetupColumn("file name", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed, 0.0f, magic_enum::enum_integer(sort_by::name));
        ImGui::TableSetupColumn("size", ImGuiTableColumnFlags_WidthFixed, 0.0f, magic_enum::enum_integer(sort_by::size));
        ImGui::TableSetupColumn("last write time", ImGuiTableColumnFlags_WidthFixed, 0.0f, magic_enum::enum_integer(sort_by::time));
        ImGui::TableHeadersRow();

        /// \brief 这里进行排序
        if (auto *l_sorts_specs = dear::TableGetSortSpecs()) {
          if (l_sorts_specs->SpecsDirty) {
            this->sort_file_attr(
                magic_enum::enum_cast<sort_by>(
                    boost::numeric_cast<std::int16_t>(l_sorts_specs->Specs[0].ColumnUserID)
                )
                    .value(),
                l_sorts_specs->Specs[0].SortDirection == ImGuiSortDirection_Ascending
            );
          }
          l_sorts_specs->SpecsDirty = false;
        }

        for (auto [l_index, k_p] : p_i->path_list | ranges::views::enumerate) {
          ImGui::TableNextRow();
          /// \brief 设置文件名序列
          ImGui::TableNextColumn();
          if (dear::Selectable(k_p.show_name, k_p.has_select, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick)) {
            set_select(l_index);
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
void file_panel::set_select(std::size_t in_index) {
  auto &&k_p = p_i->path_list[in_index];
  if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_::ImGuiMouseButton_Left) && k_p.is_dir) {  /// \brief 双击函数
    p_i->begin_fun_list.emplace_back(
        [=, in_path = k_p.path]() {
          this->scan_director(in_path);
        }
    );
  } else /*(imgui::IsMouseClicked(ImGuiMouseButton_::ImGuiMouseButton_Left))*/ {  /// \brief 单击函数
    /// \brief 多选时的方法
    if (p_i->p_flags_[2]) {
      auto &k_io = imgui::GetIO();
      if (k_io.KeyCtrl)
        k_p.has_select = !(k_p.has_select);
      else if (k_io.KeyShift) {
        ranges::for_each(
            p_i->path_list |
                ranges::views::slice(boost::numeric_cast<std::size_t>(std::min(in_index, p_i->select_index)), boost::numeric_cast<std::size_t>(std::min(std::max(in_index, p_i->select_index) + 1, p_i->path_list.size()))),
            [](path_info &in_attr) {
              in_attr.has_select = true;
            }
        );

      } else {
        ranges::for_each(p_i->path_list, [](auto &in) {
          in.has_select = false;
        });
        k_p.has_select = true;
      }
    } else {  /// \brief 单选
      ranges::for_each(p_i->path_list, [](auto &in) {
        in.has_select = false;
      });
      k_p.has_select = true;
    }
    generate_buffer(in_index);
    p_i->select_index = in_index;
  }
}
void file_panel::render_buffer() {
  ImGui::InputText(*p_i->buffer.gui_name, &(p_i->buffer.data));
}
void file_panel::render_filter() {
  dear::Combo{*p_i->filter_list.gui_name, p_i->filter_list.show_str.c_str()} && [&]() {
    for (auto &k_f : p_i->filter_list.data) {
      if (dear::Selectable(k_f)) {
        p_i->filter_list.show_str = k_f;
        p_i->begin_fun_list.emplace_back([this]() {
          scan_director(p_i->p_pwd);
        });
      }
    }
  };
}
void file_panel::button_ok() {
  if (imgui::Button("ok")) {
    ImGui::CloseCurrentPopup();
    this->succeeded();
  }
}
void file_panel::button_cancel() {
  if (imgui::Button("cancel")) {
    ImGui::CloseCurrentPopup();
  }
}
void file_panel::generate_buffer(std::size_t in_index) {
  if (ranges::any_of(p_i->path_list, [](const path_info &in) -> bool {
        return in.has_select;
      })) {
    auto l_size = ranges::count_if(p_i->path_list, [](const path_info &in) -> bool {
      return in.has_select;
    });
    if (l_size > 1) {
      p_i->buffer.data = fmt::format("选中了 {} 个路径", l_size);
    } else {
      p_i->buffer.data = p_i->path_list[in_index].path.filename().generic_string();
    }
  } else {
    p_i->buffer.data.clear();
  }
}
FSys::path file_panel::get_select() {
  FSys::path result{};
  if (std::any_of(p_i->path_list.begin(), p_i->path_list.end(), [](const path_info &in) {
        return in.has_select;
      }))
    result = p_i->path_list.at(p_i->select_index)
                 .path;
  else
    result = p_i->p_pwd;

  //  if (p_i->p_flags_[1] && !p_i->buffer.data.empty()) {
  //    result /= p_i->buffer.data;
  //  }

  return result;
}
std::vector<FSys::path> file_panel::get_selects() {
  std::vector<FSys::path> result{};
  result = p_i->path_list |
           ranges::views::filter([](const path_info &in_attr) -> bool {
             return in_attr.has_select;
           }) |
           ranges::views::transform([](const path_info &in_attr) {
             return in_attr.path;
           }) |
           ranges::to_vector;

  if (result.empty())
    result.emplace_back(p_i->p_pwd);

  return result;
}
file_panel &file_panel::async_read(one_fun &&in_fun) {
  p_i->call_fun = [in_fun = std::move(in_fun)](const std::vector<FSys::path> &in) {
    if (!in.empty())
      in_fun(in.front());
  };
  return *this;
}
file_panel &file_panel::async_read(mult_fun &&in_fun) {
  p_i->call_fun = in_fun;
  return *this;
}
void file_panel::set_attr() {
  ImGui::OpenPopup(title().data());
  ImGui::SetNextWindowSize({640, 360});
}
std::int32_t file_panel::flags() const {
  return ImGuiWindowFlags_NoSavedSettings;
}
file_panel::~file_panel() = default;

file_panel::dialog_args::dialog_args()
    : p_flags(),
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

file_panel::dialog_args &file_panel::dialog_args::add_filter(const std::string &in_filter) {
  filter.emplace_back(in_filter);
  return *this;
}

file_panel::dialog_args &file_panel::dialog_args::set_pwd(const FSys::path &in_pwd) {
  pwd = in_pwd;
  return *this;
}

file_panel::dialog_args &file_panel::dialog_args::use_default_pwd() {
  pwd = g_reg()->ctx().emplace<default_pwd>().pwd;
  return *this;
}
file_panel::dialog_args &file_panel::dialog_args::multiple(bool in_multiple) {
  p_flags[2] = in_multiple;

  return *this;
}
}  // namespace doodle::gui
