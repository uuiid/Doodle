//
// Created by TD on 2021/9/22.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/base_windwos.h>
#include <bitset>
namespace doodle {
using ImGuiFileBrowserFlags = int;
class DOODLELIB_API file_dialog : public process_t<file_dialog> {
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  enum flags {
    ImGuiFileBrowserFlags_SelectDirectory   = 1 << 0,  // select directory instead of regular file
    ImGuiFileBrowserFlags_EnterNewFilename  = 1 << 1,  // allow user to enter new filename when selecting regular file
    ImGuiFileBrowserFlags_NoModal           = 1 << 2,  // file browsing window is modal by default. specify this to use a popup window
    ImGuiFileBrowserFlags_NoTitleBar        = 1 << 3,  // hide window title bar
    ImGuiFileBrowserFlags_NoStatusBar       = 1 << 4,  // hide status bar at the bottom of browsing window
    ImGuiFileBrowserFlags_CloseOnEsc        = 1 << 5,  // close file browser when pressing 'ESC'
    ImGuiFileBrowserFlags_CreateNewDir      = 1 << 6,  // allow user to create new directory
    ImGuiFileBrowserFlags_MultipleSelection = 1 << 7,  // allow user to select multiple files. this will hide ImGuiFileBrowserFlags_EnterNewFilename
  };
  using base_type  = process_t<file_dialog>;
  using one_sig    = std::shared_ptr<FSys::path>;
  using mult_sig   = std::shared_ptr<std::vector<FSys::path>>;
  using select_sig = std::variant<one_sig, mult_sig>;

  /**
   * @brief 最为齐全的参数
   * @param in_sig 传入的指针
   * @param in_title 标题
   * @param in_flags 标志
   * @param in_filters 过滤器
   * @param in_pwd 当前路径
   */
  explicit file_dialog(const select_sig& in_ptr,
                       const std::string& in_title,
                       std::int32_t in_flags,
                       const std::vector<string>& in_filters,
                       const FSys::path& in_pwd);

  /**
   * @brief 使用目录选择
   * @param in_function 传入的指针
   * @param in_title 传入的标题
   * @param in_pwd 打开时的路径
   */
  explicit file_dialog(
      const select_sig& in_function,
      const std::string& in_title);

  /**
   * @brief 使用文件选择, 需要过滤器过滤器
   * @param in_function 传入的指针
   * @param in_title
   * @param in_filters
   * @param in_pwd
   */
  explicit file_dialog(
      const select_sig& in_function,
      const std::string& in_title,
      const std::vector<string>& in_filters,
      const FSys::path& in_pwd = FSys::current_path());

  ~file_dialog() override;

  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(base_type::delta_type, void* data);
};
/**
 * @brief
 * @todo 重构 file_dialog 类
 */
class DOODLELIB_API file_panel : public modal_window<file_panel> {
  class impl;
  class path_info;
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

  using one_sig    = std::shared_ptr<FSys::path>;
  using mult_sig   = std::shared_ptr<std::vector<FSys::path>>;
  using select_sig = std::variant<one_sig, mult_sig>;

  class dialog_args {
    friend file_panel;
    select_sig out_ptr;
    dialog_flags p_flags;
    std::vector<std::string> filter;
    std::string title;
    FSys::path pwd;

   public:
    explicit dialog_args(select_sig in_out_ptr);

    dialog_args& set_use_dir(bool use_dir = true);
    dialog_args& create_file_module(bool use_create = true);
    dialog_args& set_title(std::string in_title);
    dialog_args& set_filter(const std::vector<std::string>& in_filters);
    dialog_args& add_filter(const std::string& in_filter);
    dialog_args& set_pwd(const FSys::path& in_pwd);
    dialog_args& use_default_pwd();
  };
  /**
   * @brief 创建文件选择对话框
   *
   * @param in_args 传入的参数类
   */
  explicit file_panel(const dialog_args& in_args);

  [[maybe_unused]] [[nodiscard]] std::string& title() const;
  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(const delta_type&, void* data);
};
}  // namespace doodle
