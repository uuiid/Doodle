//
// Created by TD on 2022/6/21.
//
#include "strand_gui.h"

namespace doodle {
namespace detail {
strand_gui_executor_service::strand_gui_executor_service(boost::asio::execution_context& context)
    : execution_context_service_base<strand_gui_executor_service>(context),
      mutex_(),
      stop_(false) {
}

void strand_gui_executor_service::strand_impl::ready_start() {
  static std::function<void(const boost::system::error_code& in_code)> s_fun{};
  s_fun = [&](const boost::system::error_code& in_code) {
    if (in_code == boost::asio::error::operation_aborted || service_->stop_)
      return;

    service_->loop_one();

    timer_.expires_after(doodle::chrono::seconds{1} / 60);
    timer_.async_wait(s_fun);
  };
  timer_.expires_after(doodle::chrono::seconds{1} / 60);
  timer_.async_wait(s_fun);
}
strand_gui_executor_service::strand_impl::~strand_impl() = default;

void strand_gui_executor_service::shutdown() {
  std::lock_guard l_g{mutex_};
  stop_ = true;
  for (auto&& i : impl_list_->handlers) {
    i.abort(true);
    i();
  }
  for (auto&& i : impl_list_->handlers_next) {
    i.abort(true);
    i();
  }
}
void strand_gui_executor_service::loop_one() {
  std::lock_guard l_g{mutex_};
  std::move(impl_list_->handlers_next.begin(),
            impl_list_->handlers_next.end(), std::back_inserter(impl_list_->handlers));
  impl_list_->handlers_next.clear();
  if (impl_list_->handlers.empty())
    return;
  auto l_erase_benin = std::remove_if(
      impl_list_->handlers.begin(),
      impl_list_->handlers.end(),
      [&](typename decltype(this->impl_list_->handlers)::value_type& handler) -> bool {
        return handler();
      });
  if (l_erase_benin != impl_list_->handlers.end())
    impl_list_->handlers.erase(l_erase_benin,
                               impl_list_->handlers.end());
}
void strand_gui_executor_service::show(
    const strand_gui_executor_service::implementation_type& in_impl,
    gui_process_t&& in_gui) {
  {
    std::lock_guard l_g{in_impl->service_->mutex_};
    in_impl->handlers_next.emplace_back(std::move(in_gui));
  }
}
void strand_gui_executor_service::stop(
    const strand_gui_executor_service::implementation_type& in_impl) {
  in_impl->service_->stop_ = true;
  in_impl->timer_.cancel();
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
void strand_gui::show(gui_process_t&& in_fun) {
  detail::strand_gui_executor_service::show(impl_, std::move(in_fun));
}
strand_gui::inner_executor_type strand_gui::get_inner_executor() const BOOST_ASIO_NOEXCEPT  {
  return executor_;
}

}  // namespace doodle
