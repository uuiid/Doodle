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
  virtual void async_run(
      const entt::handle& in_handle, const time_point_wrap& in_begin, const time_point_wrap& in_end,
      const call_type_ptr& in_call_type_ptr
  ) = 0;

 public:
  attendance_interface()                                                                 = default;
  virtual ~attendance_interface()                                                        = default;
  virtual void set_user(const entt::handle& in_handle)                                   = 0;
  virtual void set_range(const time_point_wrap& in_begin, const time_point_wrap& in_end) = 0;

  /**
   * 异步获取时钟, 主要针对网络数据
   * @tparam CompletionHandler 传入的处理句柄
   * @param in_handle 传入的用户句柄
   * @param in_begin 获取开始时间
   * @param in_end 结束时间
   * @param in_completion  传入的处理句柄实例
   * @return 根据句柄进行返回
   */
  template <typename CompletionHandler>
  auto async_get_work_clock(
      const entt::handle& in_handle, const time_point_wrap& in_begin, const time_point_wrap& in_end,
      CompletionHandler&& in_completion
  ) {
    using call_type = void(const boost::system::error_code&, const doodle::business::work_clock&);
    return boost::asio::async_initiate<CompletionHandler, call_type>(
        [this, in_handle, in_begin, in_end](auto&& in_completion_handler) {
          auto l_call = std::make_shared<std::function<call_type>>(in_completion_handler);
          async_run(in_handle, in_begin, in_end, l_call);
        },
        in_completion
    );
  }
};

}  // namespace doodle::business::detail
