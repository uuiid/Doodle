//
// Created by TD on 2022/9/16.
//

#include "attendance.h"

#include <doodle_core/lib_warp/std_warp.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/time_tool/work_clock.h>

#include <doodle_dingding/metadata/dingding_tool.h>

#include <fmt/chrono.h>
#include <magic_enum.hpp>
namespace doodle {
namespace dingding {
namespace attendance {

namespace query {

void to_json(nlohmann::json& nlohmann_json_j, const get_day_data& nlohmann_json_t) {
  /// 时间需要特殊处理

  nlohmann_json_j["workDateFrom"] = tool::print_dingding_time(nlohmann_json_t.workDateFrom);
  nlohmann_json_j["workDateTo"]   = tool::print_dingding_time(nlohmann_json_t.workDateTo);

  nlohmann_json_j["userIdList"]   = nlohmann_json_t.userIdList;
  nlohmann_json_j["offset"]       = nlohmann_json_t.offset;
  nlohmann_json_j["limit"]        = nlohmann_json_t.limit;
  nlohmann_json_j["isI18n"]       = nlohmann_json_t.isI18n;
}
void from_json(const nlohmann::json& nlohmann_json_j, get_day_data& nlohmann_json_t) {
  /// 时间需要特殊处理
  nlohmann_json_t.workDateFrom = tool::parse_dingding_time(nlohmann_json_j.at("workDateFrom"));
  nlohmann_json_t.workDateTo   = tool::parse_dingding_time(nlohmann_json_j.at("workDateTo"));

  nlohmann_json_j.at("userIdList").get_to(nlohmann_json_t.userIdList);
  nlohmann_json_j.at("offset").get_to(nlohmann_json_t.offset);
  nlohmann_json_j.at("limit").get_to(nlohmann_json_t.limit);
  nlohmann_json_j.at("isI18n").get_to(nlohmann_json_t.isI18n);
}

void to_json(nlohmann::json& nlohmann_json_j, const get_update_data& nlohmann_json_t) {
  nlohmann_json_j["work_date"] = tool::print_dingding_time(nlohmann_json_t.work_date);
  nlohmann_json_j["userid"]    = nlohmann_json_t.userid;
}
void from_json(const nlohmann::json& nlohmann_json_j, get_update_data& nlohmann_json_t) {
  nlohmann_json_t.work_date = tool::parse_dingding_time(nlohmann_json_j.at("work_date"));
  nlohmann_json_j.get_to(nlohmann_json_t.userid);
}
}  // namespace query

void attendance::add_clock_data(doodle::business::work_clock& in_clock) const {
  ranges::for_each(attendance_result_list, [&](const attendance_result& in_r) {
    switch (in_r.check_type) {
      case detail::check_type::OnDuty:
        break;
      case detail::check_type::OffDuty:
        break;
    }
  });
}

void to_json(nlohmann::json& nlohmann_json_j, const attendance::attendance_result& nlohmann_json_t) {
  nlohmann_json_j["record_id"]       = nlohmann_json_t.record_id;
  nlohmann_json_j["source_type"]     = magic_enum::enum_name(nlohmann_json_t.source_type);
  nlohmann_json_j["plan_check_time"] = nlohmann_json_t.plan_check_time;
  nlohmann_json_j["class_id"]        = nlohmann_json_t.class_id;
  nlohmann_json_j["location_method"] = nlohmann_json_t.location_method;
  nlohmann_json_j["location_result"] = nlohmann_json_t.location_result;
  if (!nlohmann_json_t.outside_remark.empty()) nlohmann_json_j["outside_remark"] = nlohmann_json_t.outside_remark;
  nlohmann_json_j["plan_id"]         = nlohmann_json_t.plan_id;
  nlohmann_json_j["user_address"]    = nlohmann_json_t.user_address;
  nlohmann_json_j["group_id"]        = nlohmann_json_t.group_id;
  nlohmann_json_j["user_check_time"] = nlohmann_json_t.user_check_time;
  if (!nlohmann_json_t.procInst_id.empty()) nlohmann_json_j["procInst_id"] = nlohmann_json_t.procInst_id;
  nlohmann_json_j["check_type"]  = nlohmann_json_t.check_type;
  nlohmann_json_j["time_result"] = nlohmann_json_t.time_result;
}
void from_json(const nlohmann::json& nlohmann_json_j, attendance::attendance_result& nlohmann_json_t) {
  if (nlohmann_json_j.contains("record_id")) nlohmann_json_j.at("record_id").get_to(nlohmann_json_t.record_id);
  auto l_soure_type = nlohmann_json_j.at("source_type").get<std::string>();
  if (auto l_enum = magic_enum::enum_cast<detail::source_type>(l_soure_type); l_enum) {
    nlohmann_json_t.source_type = *l_enum;
  } else {
    DOODLE_LOG_INFO("无法找到 {} 对应的枚举变量", l_soure_type);
  }
  nlohmann_json_t.plan_check_time = tool::parse_dingding_time(nlohmann_json_j.at("plan_check_time"));
  if (nlohmann_json_j.contains("class_id")) nlohmann_json_j.at("class_id").get_to(nlohmann_json_t.class_id);
  if (nlohmann_json_j.contains("location_method"))
    nlohmann_json_j.at("location_method").get_to(nlohmann_json_t.location_method);
  auto l_location_result = nlohmann_json_j.at("location_result").get<std::string>();
  if (auto l_enum = magic_enum::enum_cast<doodle::dingding::attendance::detail::location_result>(l_location_result);
      l_enum) {
    nlohmann_json_t.location_result = *l_enum;
  } else {
    DOODLE_LOG_INFO("无法找到 {} 对应的枚举变量", l_location_result);
  }
  if (nlohmann_json_j.contains("outside_remark"))
    nlohmann_json_t.outside_remark = nlohmann_json_j.at("outside_remark").get<std::string>();
  nlohmann_json_j.at("plan_id").get_to(nlohmann_json_t.plan_id);
  if (nlohmann_json_j.contains("user_address")) nlohmann_json_j.at("user_address").get_to(nlohmann_json_t.user_address);
  nlohmann_json_j.at("group_id").get_to(nlohmann_json_t.group_id);
  nlohmann_json_t.user_check_time = tool::parse_dingding_time(nlohmann_json_j.at("user_check_time"));
  if (nlohmann_json_j.contains("procInst_id")) nlohmann_json_j.at("procInst_id").get_to(nlohmann_json_t.procInst_id);

  auto l_check_type = nlohmann_json_j.at("check_type").get<std::string>();
  if (auto l_enum = magic_enum::enum_cast<doodle::dingding::attendance::detail::check_type>(l_check_type); l_enum)
    nlohmann_json_t.check_type = *l_enum;
  else
    DOODLE_LOG_INFO("无法找到 {} 对应的枚举变量", l_check_type);

  auto l_time_result = nlohmann_json_j.at("time_result").get<std::string>();
  if (auto l_enum = magic_enum::enum_cast<doodle::dingding::attendance::detail::time_result>(l_time_result); l_enum)
    nlohmann_json_t.time_result = *l_enum;
  else
    DOODLE_LOG_INFO("无法找到 {} 对应的枚举变量", l_time_result);
}

void to_json(nlohmann::json& nlohmann_json_j, const attendance::approve_for_open& nlohmann_json_t) {
  nlohmann_json_j["duration_unit"] = nlohmann_json_t.duration_unit;
  nlohmann_json_j["duration"]      = nlohmann_json_t.duration;
  nlohmann_json_j["sub_type"]      = nlohmann_json_t.sub_type;
  nlohmann_json_j["tag_name"]      = nlohmann_json_t.tag_name;
  nlohmann_json_j["procInst_id"]   = nlohmann_json_t.procInst_id;
  nlohmann_json_j["begin_time"]    = nlohmann_json_t.begin_time;
  nlohmann_json_j["biz_type"]      = nlohmann_json_t.biz_type;
  nlohmann_json_j["end_time"]      = nlohmann_json_t.end_time;
  nlohmann_json_j["gmt_finished"]  = nlohmann_json_t.gmt_finished;
}

void from_json(const nlohmann::json& nlohmann_json_j, attendance::approve_for_open& nlohmann_json_t) {
  if (nlohmann_json_j.contains("duration_unit"))
    nlohmann_json_j.at("duration_unit").get_to(nlohmann_json_t.duration_unit);
  nlohmann_json_j.at("duration").get_to(nlohmann_json_t.duration);
  nlohmann_json_j.at("sub_type").get_to(nlohmann_json_t.sub_type);
  nlohmann_json_j.at("tag_name").get_to(nlohmann_json_t.tag_name);
  nlohmann_json_j.at("procInst_id").get_to(nlohmann_json_t.procInst_id);
  nlohmann_json_t.begin_time = tool::parse_dingding_time(nlohmann_json_j.at("begin_time"));

  auto l_bix_type            = nlohmann_json_j.at("biz_type").get<std::int32_t>();
  if (auto l_enum = magic_enum::enum_cast<doodle::dingding::attendance::detail::approve_type>(l_bix_type); l_enum)
    nlohmann_json_t.biz_type = *l_enum;
  else
    DOODLE_LOG_INFO("无法找到 {} 对应的枚举变量", l_bix_type);
  nlohmann_json_t.end_time     = tool::parse_dingding_time(nlohmann_json_j.at("end_time"));

  nlohmann_json_t.gmt_finished = tool::parse_dingding_time(nlohmann_json_j.at("gmt_finished"));
}

void to_json(nlohmann::json& nlohmann_json_j, const attendance& nlohmann_json_t) {
  nlohmann_json_j["work_date"]              = tool::print_dingding_time(nlohmann_json_t.work_date);
  nlohmann_json_j["attendance_result_list"] = nlohmann_json_t.attendance_result_list;
  nlohmann_json_j["userid"]                 = nlohmann_json_t.userid;
  nlohmann_json_j["approve_list"]           = nlohmann_json_t.approve_list;
  nlohmann_json_j["corpId"]                 = nlohmann_json_t.corpId;
  //  nlohmann_json_j["check_record_list"]      = nlohmann_json_t.check_record_list;
}

void from_json(const nlohmann::json& nlohmann_json_j, attendance& nlohmann_json_t) {
  nlohmann_json_t.work_date = tool::parse_dingding_time(nlohmann_json_j.at("work_date"));
  nlohmann_json_j.at("attendance_result_list").get_to(nlohmann_json_t.attendance_result_list);
  nlohmann_json_j.at("userid").get_to(nlohmann_json_t.userid);
  nlohmann_json_j.at("approve_list").get_to(nlohmann_json_t.approve_list);
  nlohmann_json_j.at("corpId").get_to(nlohmann_json_t.corpId);
  //  nlohmann_json_j.at("check_record_list").get_to(nlohmann_json_t.check_record_list);
}

void to_json(nlohmann::json& nlohmann_json_j, const day_data& nlohmann_json_t) {
  nlohmann_json_j["sourceType"]    = nlohmann_json_t.sourceType;
  nlohmann_json_j["baseCheckTime"] = tool::print_to_int(nlohmann_json_t.baseCheckTime);
  nlohmann_json_j["userCheckTime"] = tool::print_to_int(nlohmann_json_t.userCheckTime);

  if (nlohmann_json_t.procInstId) nlohmann_json_j["procInstId"] = *nlohmann_json_t.procInstId;
  if (nlohmann_json_t.approveId) nlohmann_json_j["approveId"] = *nlohmann_json_t.approveId;

  nlohmann_json_j["locationResult"] = nlohmann_json_t.locationResult;
  nlohmann_json_j["timeResult"]     = nlohmann_json_t.timeResult;
  nlohmann_json_j["checkType"]      = nlohmann_json_t.checkType;
  nlohmann_json_j["userId"]         = nlohmann_json_t.userId;
  nlohmann_json_j["workDate"]       = nlohmann_json_t.workDate;

  nlohmann_json_j["workDate"] =
      chrono::floor<chrono::seconds>(nlohmann_json_t.workDate.get_local_time()).time_since_epoch().count();
  ;
  nlohmann_json_j["recordId"] = nlohmann_json_t.recordId;
  nlohmann_json_j["planId"]   = nlohmann_json_t.planId;
  nlohmann_json_j["groupId"]  = nlohmann_json_t.groupId;
  nlohmann_json_j["id"]       = nlohmann_json_t.id;
}
void from_json(const nlohmann::json& nlohmann_json_j, day_data& nlohmann_json_t) {
  nlohmann_json_j.at("sourceType").get_to(nlohmann_json_t.sourceType);
  nlohmann_json_t.baseCheckTime =
      doodle::dingding::detail::tool::parse_dingding_Date(nlohmann_json_j.at("baseCheckTime"));
  nlohmann_json_t.userCheckTime =
      doodle::dingding::detail::tool::parse_dingding_Date(nlohmann_json_j.at("userCheckTime"));

  if (nlohmann_json_t.procInstId) nlohmann_json_t.procInstId = nlohmann_json_j.at("procInstId").get<std::string>();
  if (nlohmann_json_t.approveId) nlohmann_json_t.approveId = nlohmann_json_j.at("approveId").get<std::string>();

  nlohmann_json_j.at("locationResult").get_to(nlohmann_json_t.locationResult);
  nlohmann_json_j.at("timeResult").get_to(nlohmann_json_t.timeResult);
  nlohmann_json_j.at("checkType").get_to(nlohmann_json_t.checkType);
  nlohmann_json_j.at("userId").get_to(nlohmann_json_t.userId);
  nlohmann_json_t.workDate = doodle::dingding::detail::tool::parse_dingding_Date(nlohmann_json_j.at("workDate"));
  nlohmann_json_j.at("recordId").get_to(nlohmann_json_t.recordId);
  nlohmann_json_j.at("planId").get_to(nlohmann_json_t.planId);
  nlohmann_json_j.at("groupId").get_to(nlohmann_json_t.groupId);
  nlohmann_json_j.at("id").get_to(nlohmann_json_t.id);
}

}  // namespace attendance

}  // namespace dingding
}  // namespace doodle
