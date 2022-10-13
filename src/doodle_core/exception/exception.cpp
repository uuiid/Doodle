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
  switch (num_to_enum<error_enum>(ev)) {
    case error_enum::success:
      return "no error"s;
    case error_enum::sqlite3_save_error:
      return "sqlite3 error"s;
    case error_enum::file_copy_error:
      return "file copy error"s;

    case error_enum::component_missing_error:
      return "component missing error"s;
    case error_enum::invalid_handle:
      return "Invalid handle"s;
    case error_enum::file_not_exists:
      return "file not exists"s;
    case error_enum::nullptr_error:
      return "空指针错误"s;
    case error_enum::null_string:
      return "必要字符串为空"s;
    default:
      return fmt::format("unknown error {}", ev);
  }
}

const bsys::error_category& doodle_category::get() {
  const static doodle_category l_doodle_category{};
  return l_doodle_category;
}
bsys::error_condition doodle_category::default_error_condition(int ev) const noexcept {
  return error_category::default_error_condition(ev);
}

bsys::error_code make_error_code(error_enum e) {
  return bsys::error_code{enum_to_num(e), doodle_category::get()};
  //  return boost::system::error_code{enum_to_num(e), doodle_category{}};
}



}  // namespace doodle
