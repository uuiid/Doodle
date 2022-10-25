//
// Created by TD on 2022/9/14.
//

#pragma once

#include <boost/system.hpp>
namespace doodle {
namespace dingding {
namespace bsys = boost::system;
class dingding_category : public bsys::error_category {
 public:
  const char* name() const noexcept;

  std::string message(int ev) const;

  bool failed(int ev) const noexcept;

  bsys::error_condition default_error_condition(int ev) const noexcept;

  static const bsys::error_category& get();
};

}  // namespace dingding
}  // namespace doodle
