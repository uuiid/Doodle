//
// Created by TD on 2021/9/17.
//

#pragma once

#include <boost/hana.hpp>
#include <memory>
namespace doodle {
template <class ClassIn, class... Args>
std::shared_ptr<ClassIn> make_shared_(Args&&... in_args) {
  constexpr static auto has_make_this =
      boost::hana::is_valid(
          [](Args&&... in_args)
              -> decltype(ClassIn::make_this_shared(
                  std::forward<Args>(in_args)...)) {});
  using has_make_this_v = decltype(has_make_this(std::forward<Args>(in_args)...));
  if constexpr (has_make_this_v{}) {
    return ClassIn::make_this_shared(std::forward<Args>(in_args)...);
  } else {
    return std::make_shared<ClassIn>(std::forward<Args>(in_args)...);
  }
};
}  // namespace doodle
