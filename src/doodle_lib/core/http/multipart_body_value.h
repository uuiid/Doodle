//
// Created by TD on 25-1-7.
//

#pragma once
#include "doodle_core/core/core_set.h"

#include <doodle_lib/core/http/http_content_type.h>

namespace doodle::http::multipart_body_impl {
struct part_value_type {
  std::string name{};
  std::string file_name{};
  detail::content_type content_type{detail::content_type::unknown};
  std::variant<std::string, FSys::path> body_{};
};
struct value_type_impl {
  std::vector<part_value_type> parts_{};

  nlohmann::json to_json() const {
    nlohmann::json l_json{};
    for (auto&& i : parts_) {
      std::visit(
          overloaded{
              [&](const FSys::path&) {},
              [&](const std::string& in_body) {
                if (nlohmann::json::accept(in_body)) {
                  l_json[i.name] = nlohmann::json::parse(in_body);
                } else
                  l_json[i.name] = in_body;
              },
          },
          i.body_
      );
    }
    return l_json;
  }
  std::vector<FSys::path> get_files() const {
    std::vector<FSys::path> l_result{};
    for (auto&& i : parts_) {
      std::visit(
          overloaded{
              [&](const FSys::path& in_path) { l_result.emplace_back(in_path); },
              [&](const std::string&) {},
          },
          i.body_
      );
    }
    return l_result;
  }
};

}  // namespace doodle::http::multipart_body_impl