//
// Created by TD on 2021/9/22.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/lib_warp/imgui_warp.h>

#include <utility>

namespace doodle {
class DOODLELIB_API open_file_dialog
    : public std::enable_shared_from_this<open_file_dialog> {
  std::string p_vKey;

 public:
  template <class... Args>
  open_file_dialog(
      std::string vKey,
      Args&&... in_args)
      : std::enable_shared_from_this<open_file_dialog>(),
        p_vKey(std::move(vKey)) {
    ImGuiFileDialog::Instance()->OpenModal(
        p_vKey.c_str(),
        std::forward<Args>(in_args)...);
  };

  void show(const std::function<void(const std::vector<FSys::path>&)>& in_fun);
};
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
  using one_sig    = std::function<void(const FSys::path& in_select_path)>;
  using mult_sig   = std::function<void(const std::vector<FSys::path>& in_select_path)>;
  using select_sig = std::variant<one_sig, mult_sig>;

  /**
   * @brief 最为齐全的参数
   * @param in_sig 信号
   * @param in_title 标题
   * @param in_flags 标志
   * @param in_filters 过滤器
   * @param in_pwd 当前路径
   */
  explicit file_dialog(const select_sig& in_sig,
                       const std::string& in_title,
                       std::int32_t in_flags,
                       const std::vector<string>& in_filters,
                       const FSys::path& in_pwd);

  /**
   * @brief 这个是传入时 使用单个文件并且使用目录的对话框, 所以不需要传入过滤器
   * 并且使用 ImGuiFileBrowserFlags_SelectDirectory
   * @param in_function 传入的回调函数
   * @param in_title 传入的标题
   * @param in_pwd 打开时的路径
   */
  explicit file_dialog(
      const select_sig& in_function,
      const std::string& in_title,
      const FSys::path& in_pwd = FSys::current_path());

  /**
   * @brief 传入时使用单文件目录选择, 默认过滤器
   * @param in_function
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

}  // namespace doodle
