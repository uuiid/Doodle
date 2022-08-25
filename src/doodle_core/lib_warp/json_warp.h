//
// Created by TD on 2021/6/25.
//

#pragma once

#include <doodle_core/lib_warp/boost_uuid_warp.h>
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
struct [[maybe_unused]] adl_serializer<std::chrono::duration<Rep, Period>> {
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
struct [[maybe_unused]] adl_serializer<std::chrono::time_point<Clock, Duration>> {
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
struct [[maybe_unused]] adl_serializer<std::optional<T>> {
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
struct [[maybe_unused]] adl_serializer<boost::uuids::uuid> {
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

template <class... Types>
struct [[maybe_unused]] adl_serializer<std::variant<Types...>> {
  using var_t   = std::variant<Types...>;
  using index_t = std::size_t;
  static void to_json(json& j, const var_t& in_variant) {
    j["index"] = in_variant.index();
    std::visit([&](const auto& in_item) {
      j["data"] = in_item;
    },
               in_variant);
  }

  static void from_json(const json& j, var_t& in_variant) {
    auto l_index = j["index"].template get<index_t>();
    if (l_index >= std::variant_size_v<var_t>) {
      throw std::runtime_error{"变体值索引错误"};
    }
    get_variant_value(j, in_variant, l_index, std::index_sequence_for<Types...>{});
  }
  /**
   * @brief 使用c++编译时索引序列进行变体数据读取
   * @tparam Index 编译时数
   * @param j json 对象
   * @param in_var 传入的联合体
   * @param in_index 传入的运行时索引
   */
  template <index_t... Index>
  static void get_variant_value(const json& j, var_t& in_var, const index_t& in_index, std::index_sequence<Index...>) {
    //    in_var.emplace<Index>(j["data"].get<decltype(std::get<Index>(std::declval<var_t>()))>());

    //    (((in_index == Index)
    //          ? (void)j["data"].get_to(std::get<Index>(in_var))
    //          : void()),
    //     ...);
    (((in_index == Index)
          ? (void)in_var.emplace<Index>(j["data"].get<std::decay_t<decltype(std::get<Index>(std::declval<var_t>()))>>())
          : void()),
     ...);
  }
  //  template <int N, std::enable_if_t<N == std::variant_size_v<var_t>, void> = 0>
  //  static void variant_value(const json& j, var_t& in_var, const index_t& in_index) {
  //    throw std::runtime_error{"变体值读取错误"};
  //  };
  //
  //  template <int N, std::enable_if_t<(N < std::variant_size_v<var_t>), void> = 0>
  //  static void variant_value(const json& j, var_t& in_var, const index_t& in_index) {
  //    if (N == in_index) {
  //      j["data"].template get_to(std::get<N>(in_var));
  //    } else {
  //      variant_value<N + 1>(j, in_var, in_index);
  //    }
  //  }
};

}  // namespace nlohmann

#include <doodle_core/lib_warp/detail/bit_set.h>
