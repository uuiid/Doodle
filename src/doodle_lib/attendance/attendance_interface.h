//
// Created by TD on 2022/10/21.
//

#pragma once
#include <doodle_core/time_tool/work_clock.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio/async_result.hpp>
#include <boost/asio/io_context.hpp>

namespace doodle::business::detail {

class attendance_interface {
 protected:
  using call_type_ptr =
      std::shared_ptr<std::function<void(const boost::system::error_code&, const doodle::business::work_clock&)>>;
  virtual void async_run(const call_type_ptr& in_call_type_ptr) = 0;

 public:
  attendance_interface()                                                                 = default;
  virtual ~attendance_interface()                                                        = default;
  virtual void set_user(const entt::handle& in_handle)                                   = 0;
  virtual void set_range(const time_point_wrap& in_begin, const time_point_wrap& in_end) = 0;
  virtual const doodle::business::work_clock& work_clock_attr() const                    = 0;

  template <typename CompletionHandler>
  auto async_get_work_clock(
      const entt::handle& in_handle, const time_point_wrap& in_begin, const time_point_wrap& in_end,
      CompletionHandler&& in_completion
  ) {
    using call_type = void(const boost::system::error_code&, const doodle::business::work_clock&);
    set_user(in_handle);
    set_range(in_begin, in_end);
    return boost::asio::async_initiate<CompletionHandler, call_type>(
        [this](auto&& in_completion_handler) {
          auto l_call = std::make_shared<std::function<call_type>>(in_completion_handler);
          async_run(l_call);
        },
        in_completion
    );
  }
};

}  // namespace doodle::business::detail
