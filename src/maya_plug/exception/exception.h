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

/**
 * @brief 检查maya的返回状态
 */
#define DOODLE_MAYA_CHICK(in_status) \
  if (auto&& l_m_s = in_status; !l_m_s) throw_exception(std::runtime_error{fmt::to_string(l_m_s)})

inline void maya_chick(const MStatus& in_status, ::boost::source_location const& in_loc = std::source_location::current()) {
  if (!in_status) throw_exception(std::runtime_error{fmt::to_string(in_status)}, in_loc);
}

#define DOODLE_MAYA_RETURN(in_status)                                        \
  if (auto&& l_m_s = in_status; !l_m_s) {                                    \
    l_m_s.pAPIerror(__FILE__, __LINE__);                                     \
    default_logger_raw()->log(log_loc(), level::err, fmt::to_string(l_m_s)); \
    return l_m_s;                                                            \
  }

}  // namespace doodle::maya_plug
