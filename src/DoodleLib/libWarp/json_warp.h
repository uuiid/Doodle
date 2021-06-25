//
// Created by TD on 2021/6/25.
//

#pragma once

#include <nlohmann/json.hpp>
#include <filesystem>

// partial specialization (full specialization works too)
namespace nlohmann {
template <>
struct adl_serializer<std::filesystem::path> {
  static void to_json(json& j, const std::filesystem::path& in_path) {
    j =in_path.generic_string();
  }

  static void from_json(const json& j, std::filesystem::path& in_path) {
    in_path = j.get<std::string>();
  }
};
}
