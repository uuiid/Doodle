//
// Created by TD on 2021/9/22.
//

#pragma once
#include <doodle_app/doodle_app_fwd.h>
#include <doodle_app/gui/base/base_window.h>

#include <bitset>
namespace doodle::gui {
/**
 * @brief
 */
class DOODLE_APP_API file_panel {
  class impl;
  class path_info;
  bool open{};
  enum class sort_by : std::int16_t {
    none = 0,
    name = 1,
    size = 2,
    time = 3,
  };

  std::unique_ptr<impl> p_i;

  void scan_director(const FSys::path& in_dir);
  void sort_file_attr(sort_by in_sort_by, bool in_reverse = false);
  void set_select(std::size_t in_index);
  void render_path(bool edit);
  void render_list_path();
  void render_buffer();
  void render_filter();
  void button_ok();
  void button_cancel();
  void generate_buffer(std::size_t in_index);
  FSys::path get_select();
  std::vector<FSys::path> get_selects();
  void succeeded();

  class default_pwd {
   public:
    default_pwd();
    explicit default_pwd(FSys::path in_pwd);
    FSys::path pwd;
  };

 public:
  using dialog_flags = std::bitset<3>;

  constexpr const static dialog_flags flags_Use_dir{0x1};
  constexpr const static dialog_flags flags_Create_Name{0x2};
  constexpr const static dialog_flags flags_Multiple_Selection{0x4};

  using mult_fun = std::function<void(const std::vector<FSys::path>&)>;
  using one_fun  = std::function<void(const FSys::path&)>;

  class dialog_args {
    friend file_panel;
    dialog_flags p_flags;
    std::vector<std::string> filter;
    std::string title;
    FSys::path pwd;
    mult_fun call_fun;
    bool multiple_attr{false};

   public:
    explicit dialog_args();

    dialog_args& set_use_dir(bool use_dir = true);
    dialog_args& multiple(bool use_dir = true);
    dialog_args& create_file_module(bool use_create = true);
    dialog_args& set_title(std::string in_title);
    dialog_args& set_filter(const std::vector<std::string>& in_filters);
    dialog_args& add_filter(const std::string& in_filter);
    dialog_args& set_pwd(const FSys::path& in_pwd);
    dialog_args& use_default_pwd();
    dialog_args& async_read(one_fun&& in_fun);
    dialog_args& async_read(mult_fun&& in_fun);
  };
  /**
   * @brief 创建文件选择对话框
   *
   * @param in_args 传入的参数类
   */
  explicit file_panel(const dialog_args& in_args);
  virtual ~file_panel();

  [[nodiscard]] std::string& title() const;
  void init();

  bool render();
  void set_attr();
  std::int32_t flags() const;
};

using file_dialog = file_panel;
}  // namespace doodle::gui
