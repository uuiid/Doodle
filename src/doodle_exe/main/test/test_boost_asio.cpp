//
// Created by TD on 2022/6/17.
//
//
// Created by TD on 2022/6/14.
//
#include <doodle_lib/doodle_lib_all.h>
#include <doodle_lib/app/app.h>
#include <doodle_core/core/core_sig.h>
#include <doodle_core/metadata/metadata_cpp.h>
#include <doodle_core/metadata/time_point_wrap.h>
#include <doodle_lib/core/work_clock.h>

#include <catch.hpp>
#include <catch2/catch_approx.hpp>

using namespace doodle;

namespace detail {

// Special derived service id type to keep classes header-file only.
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



///=================================================================
class strand_executor_service
    : public execution_context_service_base<strand_executor_service> {
};

}  // namespace detail

template <typename Executor>
class strand_gui {
 public:
  typedef Executor inner_executor_type;

  strand_gui()
      : executor_(),
        impl_(strand::create_implementation(executor_)) {
  }

  typedef detail::strand_executor_service::implementation_type
      implementation_type;
  Executor executor_;
  implementation_type impl_;
};

TEST_CASE("test boost strand") {
}
