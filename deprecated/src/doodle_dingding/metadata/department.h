//
// Created by TD on 2022/9/13.
//

#pragma once
#include <doodle_dingding/doodle_dingding_fwd.h>

#include <nlohmann/json_fwd.hpp>
#include <nlohmann/json.hpp>
namespace doodle::dingding {

namespace department_ns {
class DOODLE_DINGDING_API department_query {
 public:
  std::int32_t dept_id{1};
  std::string language{"zh_CN"};

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(department_query, dept_id, language)
};
class DOODLE_DINGDING_API user_to_dep_query {
 public:
  std::string userid{};

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(user_to_dep_query, userid)
};
}  // namespace department_ns

class DOODLE_DINGDING_API department {
 public:
  department() = default;
  std::int32_t dept_id{};
  std::string name{};
  std::int32_t parent_id{};
  //  std::string source_identifier{};

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(
      department,
      dept_id,
      name,
      parent_id
  )
};

}  // namespace doodle::dingding
