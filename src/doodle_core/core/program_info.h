//
// Created by TD on 2022/9/30.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/platform/win/windows_alias.h>
namespace doodle {
class app_base;

namespace details {

class DOODLE_CORE_API program_info {
 public:
 private:
  friend class ::doodle::app_base;
  std::atomic_bool is_stop{};
  ::doodle::win::wnd_instance handle_{};
  ::doodle::win::wnd_handle parent_handle_{};
  std::string title{};

  bool use_gui{true};

 public:
  program_info();
  ~program_info() = default;
  [[nodiscard]] const std::atomic_bool& stop_attr() const;
  [[nodiscard]] ::doodle::win::wnd_instance handle_attr() const;
  void handle_attr(::doodle::win::wnd_instance in_instance);
  /**
   * 父窗口对象 , 如果正常程序为空,  如果作为插件, 则会为插件窗口
   * @return
   */
  [[nodiscard]] ::doodle::win::wnd_handle parent_windows_attr() const;
  void parent_windows_attr(::doodle::win::wnd_handle in_);
  std::string& title_attr();
  void title_attr(const std::string& in_str);

  inline void use_gui_attr(bool in_) { use_gui = in_; };
  [[nodiscard]] inline bool use_gui_attr() const { return use_gui; };
};
}  // namespace details
}  // namespace doodle
