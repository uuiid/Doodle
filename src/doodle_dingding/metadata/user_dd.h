//
// Created by TD on 2022/9/15.
//
#pragma once

#include <doodle_dingding/doodle_dingding_fwd.h>
#include <nlohmann/json.hpp>

#include <string>
namespace doodle::dingding {
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

class DOODLE_DINGDING_API find_by_mobile {
 public:
  std::string mobile{};
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(
      find_by_mobile,
      mobile
  )
};

class DOODLE_DINGDING_API get_user_info {
 public:
  std::string userid{};
  std::string language{"zh_CN"};
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(
      get_user_info,
      userid,
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
  friend void to_json(nlohmann::json& nlohmann_json_j, const user_dd& nlohmann_json_t) {
    nlohmann_json_j["userid"]       = nlohmann_json_t.userid;
    nlohmann_json_j["unionid"]      = nlohmann_json_t.unionid;
    nlohmann_json_j["name"]         = nlohmann_json_t.name;
    nlohmann_json_j["job_number"]   = nlohmann_json_t.job_number;
    nlohmann_json_j["title"]        = nlohmann_json_t.title;
    nlohmann_json_j["dept_id_list"] = nlohmann_json_t.dept_id_list;
  }
  friend void from_json(const nlohmann::json& nlohmann_json_j, user_dd& nlohmann_json_t) {
    nlohmann_json_j.at("userid").get_to(nlohmann_json_t.userid);
    if (nlohmann_json_j.contains("unionid"))
      nlohmann_json_j.at("name").get_to(nlohmann_json_t.name);
    if (nlohmann_json_j.contains("unionid"))
      nlohmann_json_j.at("unionid").get_to(nlohmann_json_t.unionid);
    if (nlohmann_json_j.contains("job_number"))
      nlohmann_json_j.at("job_number").get_to(nlohmann_json_t.job_number);
    if (nlohmann_json_j.contains("title"))
      nlohmann_json_j.at("title").get_to(nlohmann_json_t.title);
    if (nlohmann_json_j.contains("dept_id_list"))
      nlohmann_json_j.at("dept_id_list").get_to(nlohmann_json_t.dept_id_list);
  };
};
}  // namespace doodle::dingding
