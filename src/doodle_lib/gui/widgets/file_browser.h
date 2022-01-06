//
// Created by TD on 2022/1/5.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
namespace doodle {
class DOODLELIB_API file_browser {
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  enum flags_ {
    file_browser_flags_None              = 0,
    file_browser_flags_SelectDirectory   = 1 << 0,  //选择目录而不是普通文件
    file_browser_flags_EnterNewFilename  = 1 << 1,  //允许用户在选择普通文件时输入新文件名
    file_browser_flags_NoModal           = 1 << 2,  //文件浏览窗口默认为模态. 指定此项以使用弹出窗口
    file_browser_flags_NoTitleBar        = 1 << 3,  //隐藏窗口标题栏
    file_browser_flags_NoStatusBar       = 1 << 4,  //隐藏浏览窗口底部的状态栏
    file_browser_flags_CloseOnEsc        = 1 << 5,  //按'ESC'关闭文件浏览器
    file_browser_flags_CreateNewDir      = 1 << 6,  //允许用户创建新目录
    file_browser_flags_MultipleSelection = 1 << 7,  //允许用户选择多个文件。这将隐藏 ImGuiFileBrowserFlags_EnterNewFilename
  };
  using flags = std::int32_t;
  explicit file_browser(flags in_flags = 0);
  ~file_browser();
  /**
   * @brief 显示文件管理器
   */
  void open();
  /**
   * @brief 关闭文件管理器
   */
  void close();
  /**
   * @brief 设置过滤器
   *
   * @param in_vector
   */
  void set_filter(const std::vector<string>& in_vector);

  /**
   * @brief 修改标志
   *
   * @param in_flags 标志
   */
  void set_flags(flags in_flags);
  /**
   * @brief 设置标题, id会自动根据标题生成
   * @param in_string
   */
  void set_title(const string& in_string);

  /**
   * @brief 设置当前路径
  */
  void set_pwd_path(const FSys::path& in_path);

  /**
   * @brief 查询操作: 是否有选择 ok
   * @return 是否有选择 ok 并不会判断选择是否为空
   */
  [[nodiscard]] bool is_ok() const;

  /**
   * @brief 查询操作： 选择的文件
   */
  [[nodiscard]] FSys::path get_select() const;
  /**
   * @brief 查询操作： 多个选中的文件
   */
  [[nodiscard]] std::vector<FSys::path> get_selects() const;
  /**
   * @brief 查询是否显示
   * @return 是否显示
   */
  [[nodiscard]] bool is_open()const;
  void render();

 private:
  void render_path();
  void scan_director(const FSys::path& in_path);
  void render_file_list();
  void render_buffer();
  void render_filter();
};
}  // namespace doodle
