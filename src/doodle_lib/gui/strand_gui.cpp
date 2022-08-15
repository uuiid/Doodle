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
// 启用窗口拖拽导入头文件
#include <shellapi.h>

namespace doodle {
namespace detail {
strand_gui_executor_service::strand_gui_executor_service(boost::asio::execution_context& context)
    : gui_execution_context_service_base<strand_gui_executor_service>(context),
      mutex_() {
}

void strand_gui_executor_service::strand_impl::ready_start() {
  static std::function<void(const boost::system::error_code& in_code)> s_fun{};
  s_fun = [&](const boost::system::error_code& in_code) {
    if (in_code == boost::asio::error::operation_aborted || app_command_base::Get().stop_)
      return;

    service_->render_begin();
    service_->loop_one();
    service_->render_end();

    timer_.expires_after(doodle::chrono::seconds{1} / 60);
    timer_.async_wait(s_fun);
  };
  timer_.expires_after(doodle::chrono::seconds{1} / 60);
  timer_.async_wait(s_fun);
}
strand_gui_executor_service::strand_impl::~strand_impl() = default;

void strand_gui_executor_service::shutdown() {
  std::lock_guard l_g{mutex_};
  impl_list_->handlers.clear();
  impl_list_->handlers_next.clear();
}
void strand_gui_executor_service::loop_one() {
  std::lock_guard l_g{mutex_};

  if (!impl_list_->handlers_next.empty()) {
    impl_list_->handlers |= ranges::action::push_back(impl_list_->handlers_next);
    impl_list_->handlers_next.clear();
  }

  std::for_each(
      impl_list_->handlers.begin(),
      impl_list_->handlers.end(),
      [&](typename decltype(this->impl_list_->handlers)::value_type& handler) {
        handler();
      });

  impl_list_->handlers.clear();
  std::swap(impl_list_->handlers, impl_list_->handlers_next);
}

void strand_gui_executor_service::stop(
    const strand_gui_executor_service::implementation_type& in_impl) {
  in_impl->timer_.cancel();
}
void strand_gui_executor_service::render_begin() {
  MSG msg;
  while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
    /// 如果时退出消息, 直接设置停止
    if (msg.message == WM_QUIT) {
      DOODLE_LOG_INFO("开始退出");
      return;
    }
  }

  // Start the Dear ImGui frame
  ImGui_ImplDX11_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();
}
void strand_gui_executor_service::render_end() {
  // Rendering
  ImGui::Render();
  static ImVec4 clear_color             = ImVec4(0.0f, 0.0f, 0.0f, 0.00f);
  const float clear_color_with_alpha[4] = {clear_color.x * clear_color.w,
                                           clear_color.y * clear_color.w,
                                           clear_color.z * clear_color.w,
                                           clear_color.w};
  auto d3d_deve                         = &doodle::win::d3d_device::Get();
  d3d_deve->g_pd3dDeviceContext->OMSetRenderTargets(1, &d3d_deve->g_mainRenderTargetView, nullptr);
  d3d_deve->g_pd3dDeviceContext->ClearRenderTargetView(d3d_deve->g_mainRenderTargetView, clear_color_with_alpha);
  ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

  // Update and Render additional Platform Windows
  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
  }

  d3d_deve->g_pSwapChain->Present(1, 0);  // Present with vsync
                                          // g_pSwapChain->Present(0, 0); // Present without vsync
}

}  // namespace detail
boost::asio::execution_context& strand_gui::context() const BOOST_ASIO_NOEXCEPT {
  return executor_.context();
}
void strand_gui::on_work_started() const BOOST_ASIO_NOEXCEPT {
  DOODLE_LOG_INFO("开始工作")
}
void strand_gui::on_work_finished() const BOOST_ASIO_NOEXCEPT {
  DOODLE_LOG_INFO("结束工作工作")
}
void strand_gui::stop() {
  detail::strand_gui_executor_service::stop(impl_);
}
strand_gui::inner_executor_type strand_gui::get_inner_executor() const BOOST_ASIO_NOEXCEPT {
  return executor_;
}

}  // namespace doodle
