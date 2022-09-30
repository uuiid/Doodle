//
// Created by TD on 2022/9/30.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
namespace doodle {
class app_base;
namespace core {

class DOODLE_CORE_API program_info {
 public:
  using win_handle  = win::wnd_handle;
  using win_install = win::wnd_instance;

 private:
  friend class ::doodle::app_base;
  std::atomic_bool is_stop{};
  win_install handle_{};
  win_handle parent_handle_{};
  std::string title{};

 public:
  program_info()  = default;
  ~program_info() = default;
  [[nodiscard]] const std::atomic_bool& stop_attr() const;
  [[nodiscard]] win::wnd_instance handle_attr() const;
  /**
   * 这个是如果插件使用就会有, 否则一般为空
   * @return
   */
  [[nodiscard]] win_handle parent_windows_attr() const;
  std::string& title_attr();
  void title_attr(const std::string& in_str);
};

}  // namespace core
using program_info = core::program_info;
}  // namespace doodle
