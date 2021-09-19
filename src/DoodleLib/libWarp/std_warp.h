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
std::shared_ptr<ClassIn> make_shared_(Args&&... in_args) {
  //post_constructor
  constexpr auto has_make_this =
      boost::hana::is_valid(
          [](auto obj)
              -> decltype(obj.make_this_shared()) {});
  auto ptr              = std::make_shared<ClassIn>(std::forward<Args>(in_args)...);
  using has_make_this_v = decltype(has_make_this(ptr));
  if constexpr (has_make_this_v{}) {
    ptr->make_this_shared();
  }
  return ptr;
};
}  // namespace doodle
