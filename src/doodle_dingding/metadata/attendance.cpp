//
// Created by TD on 2022/9/16.
//

#include "attendance.h"
#include <doodle_core/lib_warp/std_warp.h>
#include <doodle_dingding/metadata/dingding_tool.h>
#include <fmt/chrono.h>

namespace doodle {
namespace dingding {
namespace attendance {

namespace query {
void to_json(nlohmann::json& nlohmann_json_j, const get_day_data& nlohmann_json_t) {
  /// 时间需要特殊处理

  nlohmann_json_j["workDateFrom"] = detail::tool::print_dingding_time(nlohmann_json_t.workDateFrom);
  nlohmann_json_j["workDateTo"]   = detail::tool::print_dingding_time(nlohmann_json_t.workDateTo);

  nlohmann_json_j["userIdList"]   = nlohmann_json_t.userIdList;
  nlohmann_json_j["offset"]       = nlohmann_json_t.offset;
  nlohmann_json_j["limit"]        = nlohmann_json_t.limit;
  nlohmann_json_j["isI18n"]       = nlohmann_json_t.isI18n;
}
void from_json(const nlohmann::json& nlohmann_json_j, get_day_data& nlohmann_json_t) {
  /// 时间需要特殊处理
  nlohmann_json_t.workDateFrom = detail::tool::parse_dingding_time(nlohmann_json_j.at("workDateFrom"));
  nlohmann_json_t.workDateTo   = detail::tool::parse_dingding_time(nlohmann_json_j.at("workDateTo"));

  nlohmann_json_j.at("userIdList").get_to(nlohmann_json_t.userIdList);
  nlohmann_json_j.at("offset").get_to(nlohmann_json_t.offset);
  nlohmann_json_j.at("limit").get_to(nlohmann_json_t.limit);
  nlohmann_json_j.at("isI18n").get_to(nlohmann_json_t.isI18n);
}
}  // namespace query

void to_json(nlohmann::json& nlohmann_json_j, const day_data& nlohmann_json_t) {
  nlohmann_json_j["sourceType"]    = nlohmann_json_t.sourceType;
  nlohmann_json_j["baseCheckTime"] = chrono::floor<chrono::seconds>(
                                         nlohmann_json_t.baseCheckTime.get_local_time()
  )
                                         .time_since_epoch()
                                         .count();
  nlohmann_json_j["userCheckTime"] = chrono::floor<chrono::seconds>(
                                         nlohmann_json_t.userCheckTime.get_local_time()
  )
                                         .time_since_epoch()
                                         .count();

  if (nlohmann_json_t.procInstId)
    nlohmann_json_j["procInstId"] = *nlohmann_json_t.procInstId;
  if (nlohmann_json_t.approveId)
    nlohmann_json_j["approveId"] = *nlohmann_json_t.approveId;

  nlohmann_json_j["locationResult"] = nlohmann_json_t.locationResult;
  nlohmann_json_j["timeResult"]     = nlohmann_json_t.timeResult;
  nlohmann_json_j["checkType"]      = nlohmann_json_t.checkType;
  nlohmann_json_j["userId"]         = nlohmann_json_t.userId;
  nlohmann_json_j["workDate"]       = nlohmann_json_t.workDate;

  nlohmann_json_j["workDate"]       = chrono::floor<chrono::seconds>(
                                    nlohmann_json_t.workDate.get_local_time()
  )
                                    .time_since_epoch()
                                    .count();
  ;
  nlohmann_json_j["recordId"] = nlohmann_json_t.recordId;
  nlohmann_json_j["planId"]   = nlohmann_json_t.planId;
  nlohmann_json_j["groupId"]  = nlohmann_json_t.groupId;
  nlohmann_json_j["id"]       = nlohmann_json_t.id;
}
void from_json(const nlohmann::json& nlohmann_json_j, day_data& nlohmann_json_t) {
  nlohmann_json_j.at("sourceType").get_to(nlohmann_json_t.sourceType);
  nlohmann_json_t.baseCheckTime = doodle::dingding::detail::tool::parse_dingding_Date(
      nlohmann_json_j.at("baseCheckTime")
  );
  nlohmann_json_t.userCheckTime = doodle::dingding::detail::tool::parse_dingding_Date(
      nlohmann_json_j.at("userCheckTime")
  );

  if (nlohmann_json_t.procInstId)
    nlohmann_json_t.procInstId = nlohmann_json_j.at("procInstId").get<std::string>();
  if (nlohmann_json_t.approveId)
    nlohmann_json_t.approveId = nlohmann_json_j.at("approveId").get<std::string>();

  nlohmann_json_j.at("locationResult").get_to(nlohmann_json_t.locationResult);
  nlohmann_json_j.at("timeResult").get_to(nlohmann_json_t.timeResult);
  nlohmann_json_j.at("checkType").get_to(nlohmann_json_t.checkType);
  nlohmann_json_j.at("userId").get_to(nlohmann_json_t.userId);
  nlohmann_json_t.workDate = doodle::dingding::detail::tool::parse_dingding_Date(
      nlohmann_json_j.at("workDate")
  );
  nlohmann_json_j.at("recordId").get_to(nlohmann_json_t.recordId);
  nlohmann_json_j.at("planId").get_to(nlohmann_json_t.planId);
  nlohmann_json_j.at("groupId").get_to(nlohmann_json_t.groupId);
  nlohmann_json_j.at("id").get_to(nlohmann_json_t.id);
}

}  // namespace attendance

}  // namespace dingding
}  // namespace doodle
