//
// Created by TD on 2022/6/21.
//
#include "strand_gui.h"

#include <lib_warp/imgui_warp.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

#include <platform/win/wnd_proc.h>

#include <core/app_command_base.h>
// Helper functions
#include <d3d11.h>
#include <tchar.h>

namespace doodle {

boost::asio::execution_context& strand_gui::context() const BOOST_ASIO_NOEXCEPT {
  return executor_.context();
}
void strand_gui::on_work_started() const BOOST_ASIO_NOEXCEPT {
  DOODLE_LOG_INFO("开始工作")
}
void strand_gui::on_work_finished() const BOOST_ASIO_NOEXCEPT {
  DOODLE_LOG_INFO("结束工作工作")
}
void strand_gui::stop(){
    DOODLE_LOG_INFO("strand_gui::stop")} strand_gui::inner_executor_type strand_gui::get_inner_executor() const BOOST_ASIO_NOEXCEPT {
  return executor_;
}

}  // namespace doodle
