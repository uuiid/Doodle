//
// Created by TD on 2022/7/28.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <boost/asio.hpp>
namespace doodle {
namespace process_task_ns {
namespace detail {
template <typename Type>
class service_id
    : public boost::asio::execution_context::id {
};

// Special service base class to keep classes header-file only.
template <typename Type>
class execution_context_service_base
    : public boost::asio::execution_context::service {
  using base_type_t = boost::asio::execution_context::service;

 public:
  static service_id<Type> id;

  // Constructor.
  execution_context_service_base(boost::asio::execution_context& e)
      : base_type_t(e) {
  }
};

template <typename Type>
service_id<Type> execution_context_service_base<Type>::id;
}  // namespace detail

class process_task_service
    : public doodle::process_task_ns::detail::execution_context_service_base<process_task_service> {
 public:
};
}  // namespace process_task_ns


}  // namespace doodle
