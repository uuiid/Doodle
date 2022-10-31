//
// Created by TD on 2022/9/14.
//

#pragma once

#include <boost/system.hpp>

#include <map>
namespace doodle {
namespace dingding {
namespace bsys = boost::system;
class dingding_category : public bsys::error_category {
 private:
  std::map<std::int32_t, std::string> map_err{};

 public:
  const char* name() const noexcept;

  std::string message(int ev) const;

  bool failed(int ev) const noexcept;

  void set_message(const int evcode, const std::string& in_string);

  bsys::error_condition default_error_condition(int ev) const noexcept;

  static const bsys::error_category& get();
  static dingding_category& get_();
};

}  // namespace dingding
}  // namespace doodle
