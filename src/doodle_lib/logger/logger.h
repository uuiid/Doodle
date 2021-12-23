/*
 * @Author: your name
 * @Date: 2020-10-10 13:18:58
 * @LastEditTime: 2020-10-10 13:48:07
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\logger\logger.h
 */
#pragma once

#include <DoodleConfig.h>
#include <doodle_lib_export.h>
#include <spdlog/spdlog.h>

#define DOODLE_SOURCE_LOC \
  ::spdlog::source_loc { __FILE__, __LINE__, SPDLOG_FUNCTION }

#define DOODLE_LOC \
  ::spdlog::source_loc { __FILE__, __LINE__, SPDLOG_FUNCTION }

namespace doodle {

/**
 * @brief 在程序初始化时log就最先运行, 但是输出在了临时文件位置中,
 */
class DOODLELIB_API logger_ctrl {
  std::filesystem::path p_log_path;
  std::string p_log_name;

  void init_temp_log();
  static logger_ctrl* _self;

  void init_log();

 public:
  logger_ctrl();
  virtual ~logger_ctrl();

  enum log_type {
    none   = 0,
    guiexe = 1,
    server = 2,
  };
  static logger_ctrl& get_log();

  /**
   * @brief 这个是在运行app时重新调整日志， 这样可以正确的记录在app指定的位置上
   * @param in_name log的文件名称， 不要使用路径， 路径使用 core_set::set_root 修改
   * @return 是否成功
   */
  bool set_log_name(const std::string& in_name);
  bool add_log_sink(const std::shared_ptr<spdlog::sinks::sink>& in_ptr);
  virtual void post_constructor();
};

template <class throw_T,
          class BOOL_T,
          class FormatString, class... Args>
inline void chick_true(const BOOL_T& in, const ::spdlog::source_loc& in_loc, const FormatString& fmt, Args&&... args) {
  if (!in) {
    if constexpr (sizeof...(args) > 0) {
      spdlog::log(in_loc, spdlog::level::warn, fmt, std::forward<Args>(args)...);
      throw throw_T{fmt::format(fmt::to_string_view(fmt), std::forward<Args>(args)...)};
    } else {
      spdlog::log(in_loc, spdlog::level::warn, fmt);
      throw throw_T{fmt::to_string(fmt)};
    }
  }
}
template <class throw_T, class FormatString, class... Args>
inline void chick_false(const bool& in, const ::spdlog::source_loc& in_loc, const FormatString& fmt, Args&&... args) {
  chick_true<throw_T>(!in, in_loc, fmt, args...);
}
}  // namespace doodle
#define DOODLE_LOG_DEBUG(...) \
  SPDLOG_INFO(__VA_ARGS__);

#define DOODLE_LOG_INFO(...) \
  SPDLOG_INFO(__VA_ARGS__);

#define DOODLE_LOG_WARN(...) \
  SPDLOG_WARN(__VA_ARGS__);

#define DOODLE_LOG_ERROR(...) \
  SPDLOG_ERROR(__VA_ARGS__);
#define DOODLE_CHICK_TRUE(exp, ...) \
  ::doodle::chick_true<doodle::doodle_error>(exp, DOODLE_SOURCE_LOC, __VA_ARGS__)
