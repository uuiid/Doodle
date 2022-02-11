//
// Created by TD on 2021/6/25.
//

#pragma once

#include <boost/uuid/uuid.hpp>
#include <chrono>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <optional>
// partial specialization (full specialization works too)
namespace nlohmann {
// template <>
// struct adl_serializer<std::filesystem::path> {
//   static void to_json(json& j, const std::filesystem::path& in_path) {
//     j = in_path.generic_string();
//   }
//
//   static void from_json(const json& j, std::filesystem::path& in_path) {
//     in_path = j.get<std::string>();
//   }
// };
template <class Rep, class Period>
struct adl_serializer<std::chrono::duration<Rep, Period>> {
  using time_duration = std::chrono::duration<Rep, Period>;
  static void to_json(json& j, const time_duration& in_duration) {
    auto count = in_duration.count();
    j["count"] = count;
  }
  static void from_json(const json& j, time_duration& in_duration) {
    Rep count{};
    j.at("count").get_to(count);
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

template <class T>
struct adl_serializer<std::optional<T>> {
  using opt = std::optional<T>;
  static void to_json(json& j, const opt& in_opt) {
    if (!in_opt) {
      j["nullopt"] = true;
    } else {
      j["nullopt"] = false;
      j["data"]    = *in_opt;
    }
  }
  static void from_json(const json& j, opt& in_opt) {
    if (j["nullopt"].template get<bool>()) {
      in_opt = std::nullopt;
    } else {
      in_opt.emplace();
      *in_opt = j["data"].template get<T>();
    }
  }
};
template <>
struct adl_serializer<boost::uuids::uuid> {
  static void to_json(json& j, const boost::uuids::uuid& in_uuid) {
    j["uuid"] = boost::uuids::to_string(in_uuid);
  }

  static void from_json(const json& j, boost::uuids::uuid& in_uuid) {
    if (j["uuid"].is_string()) {
      in_uuid = boost::lexical_cast<boost::uuids::uuid>(j["uuid"].get<std::string>());
    } else {
      auto k_arr = j["uuid"].get<std::array<std::uint8_t, boost::uuids::uuid::static_size()>>();
      std::copy(k_arr.begin(), k_arr.end(), std::begin(in_uuid.data));
    }
  }
};

}  // namespace nlohmann
