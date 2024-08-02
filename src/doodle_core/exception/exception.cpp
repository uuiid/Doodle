//
// Created by TD on 2022/8/18.
//
#include <doodle_core/exception/exception.h>
#include <doodle_core/lib_warp/enum_template_tool.h>

#include <boost/system.hpp>
#include <boost/system/error_code.hpp>

namespace doodle {
// namespace bsys = boost::system;

const char* doodle_category::name() const noexcept {
  static std::string name{"doodle 错误"};
  return name.c_str();
}

std::string doodle_category::message(int ev) const {
  using namespace std::literals;
  switch (ev) {
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
    case error_enum::null_string:
      return "必要字符串为空"s;
    case error_enum::bad_json_string:
      return "json解析错误"s;
    case error_enum::bad_url:
      return "url错误"s;
    case error_enum::not_find_work_class:
      return "没有找到对应的work类"s;
    case error_enum::not_allow_multi_work:
      return "不允许多个work"s;
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
/////////////////////////////////////////////////////////////////////////
const char* exit_code_category::name() const noexcept {
  static std::string name{"退出值错误"};
  return name.c_str();
}

std::string exit_code_category::message(int ev) const { return fmt::format("进程退出值 {}", ev); }

const bsys::error_category& exit_code_category::get() {
  const static exit_code_category l_exit_code_category{};
  return l_exit_code_category;
}
/////////////////////////////////////////////////////////////////////////
const char* maya_code_category::name() const noexcept {
  static std::string name{"退出值错误"};
  return name.c_str();
}

std::string maya_code_category::message(int ev) const { return fmt::format("进程退出值 {}", ev); }

const bsys::error_category& maya_code_category::get() {
  const static maya_code_category l_maya_code_category{};
  return l_maya_code_category;
}
bsys::error_code maya_enum::make_error_code(maya_error_t e) {
  return bsys::error_code{enum_to_num(e), maya_code_category::get()};
}

/////////////////////////////////////////////////////////////////////////
[[maybe_unused]] bsys::error_code error_enum::make_error_code(error_enum::error_t e) {
  return bsys::error_code{enum_to_num(e), doodle_category::get()};
  //  return boost::system::error_code{enum_to_num(e), doodle_category{}};
}
} // namespace doodle