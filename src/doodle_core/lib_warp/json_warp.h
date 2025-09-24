//
// Created by TD on 2021/6/25.
//

#pragma once

#include <doodle_core/lib_warp/boost_uuid_warp.h>
#include <doodle_core/lib_warp/chrono_fmt.h>
#include <doodle_core/lib_warp/enum_template_tool.h>

#include <boost/rational.hpp>
#include <boost/uuid/uuid.hpp>

#include <chrono>
#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <optional>

// partial specialization (full specialization works too)
namespace nlohmann {

template <class T>
struct [[maybe_unused]] adl_serializer<boost::rational<T>> {
  using rational = boost::rational<T>;
  static void to_json(json& j, const rational& in_rational) {
    j["numerator"]   = in_rational.numerator();
    j["denominator"] = in_rational.denominator();
  }
  static void from_json(const json& j, rational& in_rational) {
    in_rational = rational{j.at("numerator").template get<T>(), j.at("denominator").template get<T>()};
  }
};

template <typename Type, Type value>
struct [[maybe_unused]] adl_serializer<std::integral_constant<Type, value>> {
  static void to_json(json& j, const std::integral_constant<Type, value>& in_entity) { j = in_entity(); }
  static void from_json(const json& j, std::integral_constant<Type, value>& in_entity) {}
};

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
    j          = count;
  }
  static void from_json(const json& j, time_duration& in_duration) {
    Rep count{};
    j.get_to(count);
    in_duration = time_duration{count};
  }
};

template <>
struct [[maybe_unused]] adl_serializer<std::chrono::year_month> {
  using year_month = std::chrono::year_month;
  static void to_json(json& j, const year_month& in_duration) {
    j = fmt::format("{}-{:02}", std::int32_t{in_duration.year()}, std::uint32_t{in_duration.month()});
  }
  static void from_json(const json& j, year_month& in_duration) {
    std::istringstream l_stream{j.get_ref<const std::string&>()};
    l_stream.exceptions(std::ios::failbit);
    l_stream >> std::chrono::parse("%Y-%m", in_duration);
  }
};

template <>
struct [[maybe_unused]] adl_serializer<std::chrono::year_month_day> {
  using year_month_day = std::chrono::year_month_day;
  static void to_json(json& j, const year_month_day& in_duration) {
    j = fmt::format(
        "{}-{:02}-{:02}", std::int32_t{in_duration.year()}, std::uint32_t{in_duration.month()},
        std::uint32_t{in_duration.day()}
    );
  }
  static void from_json(const json& j, year_month_day& in_duration) {
    std::istringstream l_stream{j.get_ref<const std::string&>()};
    l_stream.exceptions(std::ios::failbit);
    l_stream >> std::chrono::parse("%Y-%m-%d", in_duration);
  }
};

template <class Clock, class Duration>
struct [[maybe_unused]] adl_serializer<std::chrono::time_point<Clock, Duration>> {
  using time_point = std::chrono::time_point<Clock, Duration>;
  static void to_json(json& j, const time_point& in_time) {
    try {
      if (in_time.time_since_epoch() <= Duration{0} && std::is_same_v<Clock, std::chrono::local_t>)
        j = "1970-01-01 00:00:00";
      else
        j = std::format("{:%F %T}", in_time);
    } catch (const fmt::format_error& in_err) {
      // j = nlohmann::json::value_t::null;
      throw nlohmann::json::other_error::create(502, in_err.what(), &j);
    }
  }

  static void from_json(const json& j, time_point& in_time) {
    auto& l_str = j.get_ref<const std::string&>();
    std::istringstream l_stream{j.get_ref<const std::string&>()};
    if constexpr (std::ratio_less_v<typename Duration::period, std::chrono::days::period>) {
      if (l_stream >> std::chrono::parse("%FT%TZ", in_time))
        ;
      else if (l_stream.clear(), l_stream.str(l_str), l_stream >> std::chrono::parse("%FT%T%Ez", in_time))
        ;
      else if (l_stream.clear(), l_stream.str(l_str), l_stream >> std::chrono::parse("%F %T", in_time))
        ;
      else if (l_stream.clear(), l_stream.str(l_str), l_stream >> std::chrono::parse("%F", in_time))
        ;
    } else
      l_stream >> std::chrono::parse("%F", in_time);
  }
};
template <class Duration>
struct [[maybe_unused]] adl_serializer<std::chrono::zoned_time<Duration>> {
  using time_point = std::chrono::zoned_time<Duration>;
  static void to_json(json& j, const time_point& in_time) {
    try {
      if (in_time.get_sys_time().time_since_epoch() <= Duration{0})
        j = "1970-01-01 00:00:00";
      else
        j = in_time.get_local_time();
    } catch (const fmt::format_error& in_err) {
      throw nlohmann::json::other_error::create(502, in_err.what(), &j);
    }
  }

  static void from_json(const json& j, time_point& in_time) {
    auto& l_str = j.get_ref<const std::string&>();
    std::istringstream in{l_str};

    if (std::chrono::time_point<std::chrono::system_clock, Duration> l_time{};
        in >> std::chrono::parse("%FT%TZ", l_time))
      in_time = time_point{std::chrono::current_zone(), l_time};
    else if (in.clear(), in.str(l_str), in >> std::chrono::parse("%FT%T%Ez", l_time))
      in_time = time_point{std::chrono::current_zone(), l_time};
    else if (std::chrono::time_point<std::chrono::local_t, Duration> l_loc_time{};
             in.clear(), in.str(l_str), in >> std::chrono::parse("%F %T", l_loc_time))
      in_time = time_point{std::chrono::current_zone(), l_loc_time};
    else if (in.clear(), in.exceptions(std::ios::failbit), in.str(l_str), in >> std::chrono::parse("%F", l_loc_time))
      in_time = time_point{std::chrono::current_zone(), l_loc_time};
    // std::chrono::local_time<Duration> l_local_time = j.get<std::chrono::local_time<Duration>>();
    // in_time = std::chrono::zoned_time<Duration>{std::chrono::current_zone(), l_local_time};
  }
};
template <class T>
struct [[maybe_unused]] adl_serializer<std::optional<T>> {
  using opt = std::optional<T>;
  static void to_json(json& j, const opt& in_opt) {
    if (!in_opt) {
      j = nlohmann::json::value_t::null;
    } else {
      j = *in_opt;
    }
  }
  static void from_json(const json& j, opt& in_opt) {
    if (!j.is_null()) in_opt.emplace(j.get<T>());
  }
};
template <>
struct [[maybe_unused]] adl_serializer<boost::uuids::uuid> {
  static void to_json(json& j, const boost::uuids::uuid& in_uuid) {
    if (in_uuid.is_nil())
      j = nlohmann::json::value_t::null;
    else
      j = boost::uuids::to_string(in_uuid);
  }

  static void from_json(const json& j, boost::uuids::uuid& in_uuid) {
    try {
      if (j.is_string() && !j.get_ref<const std::string&>().empty())
        in_uuid = boost::lexical_cast<boost::uuids::uuid>(j.get_ref<const std::string&>());
      else
        in_uuid = boost::uuids::nil_uuid();
    } catch (const boost::bad_lexical_cast& in_err) {
      throw nlohmann::json::parse_error::create(116, {}, in_err.what(), &j);
    }
  }
};

template <class... Types>
struct [[maybe_unused]] adl_serializer<std::variant<Types...>> {
  using var_t   = std::variant<Types...>;
  using index_t = std::size_t;
  static void to_json(json& j, const var_t& in_variant) {
    j["index"] = in_variant.index();
    std::visit([&](const auto& in_item) { j["data"] = in_item; }, in_variant);
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
          ? (void)in_var.template emplace<std::decay_t<decltype(std::get<Index>(std::declval<var_t>()))>>(
                j["data"].get<std::decay_t<decltype(std::get<Index>(std::declval<var_t>()))>>()
            )
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
