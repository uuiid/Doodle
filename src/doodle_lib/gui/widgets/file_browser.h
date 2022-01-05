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

  void render();
};
}  // namespace doodle
