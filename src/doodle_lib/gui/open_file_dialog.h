//
// Created by TD on 2021/9/22.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/base_windwos.h>

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

class DOODLELIB_API file_panel : public modal_window<file_panel> {
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  using one_sig    = std::shared_ptr<FSys::path>;
  using mult_sig   = std::shared_ptr<std::vector<FSys::path>>;
  using select_sig = std::variant<one_sig, mult_sig>;

  /**
   * @brief 使用文件选择, 需要过滤器过滤器
   * @param in_select_ptr 传入的指针
   * @param in_title
   * @param in_filters
   * @param in_pwd
   */
  explicit file_panel(
      const select_sig& out_select_ptr,
      const std::string& in_title,
      const std::vector<string>& in_filters,
      const FSys::path& in_pwd = {});
  /**
   * @brief 使用目录选择
   * @param out_select_ptr 传入的指针
   * @param in_title 传入的标题
   * @param in_pwd 打开时的路径
   */
  explicit file_panel(
      const select_sig& out_select_ptr,
      const std::string& in_title);

  [[maybe_unused]] std::string title() const;
  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(const delta_type&, void* data);
};
}  // namespace doodle
