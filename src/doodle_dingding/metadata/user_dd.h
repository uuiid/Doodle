//
// Created by TD on 2022/9/15.
//
#pragma once

#include <doodle_dingding/doodle_dingding_fwd.h>
#include <nlohmann/json.hpp>

#include <string>
namespace doodle {
namespace dingding {
namespace user_dd_ns {

class DOODLE_DINGDING_API dep_query {
 public:
  std::int32_t dept_id{};
  std::int32_t cursor{};
  std::int32_t size{};
  std::string order_field{"custom"};
  bool contain_access_limit{};
  std::string language{"zh_CN"};
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(
      dep_query,
      dept_id,
      cursor,
      size,
      order_field,
      contain_access_limit,
      language
  )
};
}  // namespace user_dd_ns

class DOODLE_DINGDING_API user_dd {
 public:
  std::string userid;
  std::string unionid;
  std::string name;
  //  std::string avatar;
  //  std::string state_code;
  //  std::string manager_userid;
  std::string mobile;
  //  std::string hide_mobile;
  //  std::string telephone;
  std::string job_number;
  std::string title;
  //  std::string email;
  //  std::string work_place;
  //  std::string remark;
  std::string dept_id_list;
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(
      user_dd,
      userid,
      unionid,
      name,
      job_number,
      title,
      dept_id_list
  )
};

}  // namespace dingding
}  // namespace doodle
