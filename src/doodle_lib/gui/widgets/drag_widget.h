//
// Created by TD on 2022/1/28.
//
#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {

class DOODLELIB_API drag_widget {
 public:
  using one_sig  = std::function<void(const FSys::path& in_select_path)>;
  using mult_sig = std::function<void(const std::vector<FSys::path>& in_select_path)>;
  /**
   * @brief 拖拽时接受单个文件
   * @param in_sig 回调函数
   * @param dir 是否是目录
   */
  drag_widget(const one_sig& in_sig, bool dir);
  /**
   * @brief 拖拽时接受多个文件
   * @param in_sig 回调函数
   * @param dir 是否是目录
   */
  drag_widget(const mult_sig& in_sig, bool dir);
};
}  // namespace doodle
