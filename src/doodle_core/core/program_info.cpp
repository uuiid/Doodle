//
// Created by TD on 2022/9/30.
//

#include "program_info.h"

#include <doodle_core/core/app_base.h>

namespace doodle::details {
program_info::program_info()
    : is_stop(), handle_(), parent_handle_(), title(doodle::version::build_info::get().version_str) {}

const std::atomic_bool& program_info::stop_attr() const { return is_stop; }
::doodle::win::wnd_instance program_info::handle_attr() const { return handle_; }
void program_info::handle_attr(::doodle::win::wnd_instance in_instance) { handle_ = in_instance; }
::doodle::win::wnd_handle program_info::parent_windows_attr() const { return parent_handle_; }

std::string& program_info::title_attr() { return title; }
void program_info::title_attr(const std::string& in_str) { title = in_str; }
void program_info::parent_windows_attr(::doodle::win::wnd_handle in_) { parent_handle_ = in_; }

}  // namespace doodle::details
