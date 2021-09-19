//
// Created by TD on 2021/9/17.
//

#pragma once

#include <boost/hana.hpp>
#include <memory>
namespace doodle {
namespace details {

}  // namespace details

template <class ClassIn, class... Args>
std::shared_ptr<ClassIn> new_object(Args&&... in_args) {
  // post_constructor
  constexpr auto has_make_this =
      boost::hana::is_valid(
          [](auto obj)
              -> decltype(obj.post_constructor()) {});
  auto ptr              = new_object<ClassIn>(std::forward<Args>(in_args)...);
  using has_make_this_v = decltype(has_make_this(ptr));
  if constexpr (has_make_this_v{}) {
    ptr->post_constructor();
  }
  return ptr;
};
}  // namespace doodle
