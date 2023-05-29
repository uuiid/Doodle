//
// Created by TD on 2021/12/20.
//

#pragma once
#include <doodle_core/exception/exception.h>
#include <doodle_core/lib_warp/enum_template_tool.h>
#include <doodle_core/logger/logger.h>

#include <maya_plug/fmt/fmt_warp.h>

#include <maya/MStatus.h>
#include <maya/MString.h>

namespace doodle::maya_plug {
class maya_error : public std::system_error {
 public:
  using std::system_error::system_error;
};

class maya_category : public bsys::error_category {
 public:
  const char* name() const noexcept final;

  std::string message(int ev) const final;

  bsys::error_condition default_error_condition(int ev) const noexcept final;

  static const bsys::error_category& get();
};

inline bsys::error_code make_error(const MStatus::MStatusCode& in_code) {
  return std::error_code{enum_to_num(in_code), maya_category::get()};
};
/**
 * @brief 检查maya的返回状态
 */
#define DOODLE_MAYA_CHICK(in_status)    \
  if (auto&& l_m_s = in_status; !l_m_s) \
  throw_exception(maya_error{make_error(l_m_s.statusCode()), fmt::to_string(l_m_s)})

inline void maya_chick(const MStatus& in_status, ::boost::source_location const& in_loc = BOOST_CURRENT_LOCATION) {
  if (!in_status) throw_exception(maya_error{make_error(in_status.statusCode()), fmt::to_string(in_status)}, in_loc);
}

}  // namespace doodle::maya_plug

namespace boost::system {

template <>
struct is_error_code_enum< ::MStatus::MStatusCode> : std::true_type {};
}  // namespace boost::system
namespace std {

template <>
struct is_error_code_enum< ::MStatus::MStatusCode> : std::true_type {};
}  // namespace std
