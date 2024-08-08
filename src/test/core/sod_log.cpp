//
// Created by td_main on 2023/9/7.
//
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/logger/logger.h>
template <typename... Args>
inline void log_debug(
    const doodle::logger_ptr& in_logger, fmt::format_string<Args...> in_fmt, Args&&... in_args,
    ::boost::source_location const& in_loc = std::source_location::current()
) {
  in_logger->log(
      spdlog::source_loc{in_loc.file_name(), static_cast<std::int32_t>(in_loc.line()), in_loc.function_name()},
      spdlog::level::debug, in_fmt, std::forward<Args>(in_args)...
  );
}
int core_sod_log(int, char** const) {
  //  log_debug(spdlog::default_logger(), "tset {}", "111");
  //  log_debug(spdlog::default_logger(), "tset {}", "111");
  return 0;
}