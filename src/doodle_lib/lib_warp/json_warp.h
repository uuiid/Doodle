//
// Created by TD on 2021/6/25.
//

#pragma once

#include <chrono>
#include <filesystem>
#include <nlohmann/json.hpp>

// partial specialization (full specialization works too)
namespace nlohmann {
template <>
struct adl_serializer<std::filesystem::path> {
  static void to_json(json& j, const std::filesystem::path& in_path) {
    j = in_path.generic_string();
  }

  static void from_json(const json& j, std::filesystem::path& in_path) {
    in_path = j.get<std::string>();
  }
};
template <class Rep, class Period>
struct adl_serializer<std::chrono::duration<Rep, Period>> {
  using time_duration = std::chrono::duration<Rep, Period>;
  static void to_json(json& j, const time_duration& in_duration) {
    auto count = in_duration.count();
    j["count"] = count;
  }
  static void from_json(const json& j, time_duration& in_duration) {
    Rep count{};
    j.at("count").template get_to(count);
    in_duration = time_duration{count};
  }
};

template <class Clock, class Duration>
struct adl_serializer<std::chrono::time_point<Clock, Duration>> {
  using time_point = std::chrono::time_point<Clock, Duration>;
  static void to_json(json& j, const time_point& in_time) {
    j["time_since_epoch"] = in_time.time_since_epoch();
  }

  static void from_json(const json& j, time_point& in_time) {
    Duration time_since_epoch;
    time_since_epoch = j.at("time_since_epoch").get<Duration>();
    in_time          = time_point{time_since_epoch};
  }
};

}  // namespace nlohmann
