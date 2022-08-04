//
// Created by TD on 2022/8/4.
//

#pragma once

#include <nlohmann/json.hpp>
#include <bitset>
namespace nlohmann {
template <std::size_t N>
struct [[maybe_unused]] adl_serializer<std::bitset<N>> {
  using bit_set_type = std::bitset<N>;

  static void to_json(json& j, const bit_set_type& in_set) {
    j["data"] = in_set.to_string();
  }

  static void from_json(const json& j, bit_set_type& in_set) {
    std::string l_data{};
    j.at("data").get_to(l_data);
    in_set = bit_set_type{l_data};
  }
};
}  // namespace nlohmann
