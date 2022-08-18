//
// Created by TD on 2022/8/18.
//
#include <doodle_core/exception/exception.h>
#include <boost/system.hpp>
#include <boost/system/error_code.hpp>

#include <doodle_core/lib_warp/enum_template_tool.h>
namespace doodle {
// namespace bsys = boost::system;

const char* doodle_category::name() const noexcept {
  static std::string name{"doodle 错误"};
  return name.c_str();
}
std::string doodle_category::message(int ev) const {
  using namespace std::literals;
  switch (ev) {
    case 0:
      return "no error"s;
    case 1:
      return "voltage out of range"s;
    case 2:
      return "impedance mismatch"s;

    case 31:
    case 32:
    case 33:
      return fmt::format("component {} failure", ev - 30);

    default:
      return fmt::format("unknown error {}", ev);
  }
}
char const* doodle_category::message(int ev, char* buffer, std::size_t len) const noexcept {
  auto l_str = message(ev);
  buffer     = std::copy_n(l_str.begin(), len, buffer);
  return buffer;
}
bool doodle_category::failed(int ev) const noexcept {
  return ev != 0;
}
const bsys::error_category& doodle_category::get() {
  const static doodle_category l_doodle_category{};
  return l_doodle_category;
}
bsys::error_condition doodle_category::default_error_condition(int ev) const noexcept {
  return error_category::default_error_condition(ev);
}
bsys::error_code make_error_code(error_enum e, ::boost::source_location const& in_loc) {
  return bsys::error_code{enum_to_num(e), doodle_category::get(), &in_loc};
  //  return boost::system::error_code{enum_to_num(e), doodle_category{}};
}
}  // namespace doodle
