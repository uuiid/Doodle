//
// Created by TD on 2022/6/1.
//
#pragma once

#include <doodle_core/doodle_core_fwd.h>

namespace doodle::database_n::details {

class com_data : boost::less_than_comparable<com_data> {
 public:
  com_data(entt::entity in_entt, std::uint32_t in_id, std::string in_str)
      : entt_(in_entt), com_id(in_id), json_data(std::move(in_str)) {}

  entt::entity entt_{};
  std::uint32_t com_id{};
  std::string json_data{};
  bool operator<(const com_data& in_rhs) const;
};

}  // namespace doodle::database_n::details
