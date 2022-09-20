//
// Created by TD on 2022/9/19.
//

#include "dingding_tool.h"
#include <doodle_core/metadata/time_point_wrap.h>
#include <nlohmann/json.hpp>

#include <date/tz.h>


#include <fmt/chrono.h>


//namespace fmt{
//template <typename Char, typename Duration>
//struct formatter<std::chrono::time_point<date::local_t, Duration>,
//                 Char> : formatter<std::tm, Char> {
//  FMT_CONSTEXPR formatter() {
//    this->do_parse(default_specs,
//                   default_specs + sizeof(default_specs) / sizeof(Char));
//  }
//
//  template <typename ParseContext>
//  FMT_CONSTEXPR auto parse(ParseContext& ctx) -> decltype(ctx.begin()) {
//    return this->do_parse(ctx.begin(), ctx.end(), true);
//  }
//
//  template <typename FormatContext>
//  auto format(std::chrono::time_point<std::chrono::system_clock> val,
//              FormatContext& ctx) const -> decltype(ctx.out()) {
//    return formatter<std::tm, Char>::format(localtime(val), ctx);
//  }
//
//  static constexpr const Char default_specs[] = {'%', 'F', ' ', '%', 'T'};
//};
//}


namespace doodle::dingding::detail {
time_point_wrap tool::parse_dingding_time(const nlohmann::json& time_obj) {
  chrono::local_seconds time;
  std::istringstream l_time{time_obj.get<std::string>()};
  l_time >> chrono::parse("%Y-%m-%d %H:%M:%S", time);
  return time_point_wrap{time};
}
time_point_wrap tool::parse_dingding_Date(const nlohmann::json& time_obj) {
  chrono::microseconds l_time{time_obj.get<chrono::microseconds::rep>()};
  return time_point_wrap{chrono::sys_time<chrono::microseconds>{l_time}};
}
std::string tool::print_dingding_time(const time_point_wrap& in_time) {
  return fmt::format("{:%Y-%m-%d %H:%M:%S}", chrono::clock_cast<chrono::system_clock>(in_time.get_local_time()));
}

}  // namespace doodle::dingding::detail
