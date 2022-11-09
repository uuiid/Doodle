//
// Created by TD on 2022/9/16.
//

#include "attendance.h"

#include "doodle_core/core/chrono_.h"
#include "doodle_core/exception/exception.h"
#include "doodle_core/metadata/time_point_wrap.h"
#include <doodle_core/lib_warp/std_warp.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/time_tool/work_clock.h>

#include <doodle_dingding/metadata/dingding_tool.h>

#include <boost/numeric/conversion/cast.hpp>

#include "metadata/attendance.h"
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fmt/chrono.h>
#include <magic_enum.hpp>
#include <range/v3/algorithm/for_each.hpp>
#include <string>
#include <tuple>
#include <utility>

namespace doodle::dingding::attendance {

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

namespace {

time_point_wrap time_ounding(const time_point_wrap& in) {
  auto&& [l_y, l_m, l_d, l_h, l_mm, l_s] = in.compose();
  auto l_m1 = boost::numeric_cast<std::uint16_t>(std::round(boost::numeric_cast<std::float_t>(l_mm) / 10) * 10);
  return time_point_wrap{l_y, l_m, l_d, l_h, l_m1, l_s};
}

time_point_wrap time_13_30_to_12_00(const time_point_wrap& in) {
  auto&& [l_y, l_m, l_d, l_h, l_mm, l_s] = in.compose();
  if (l_h == 13 && l_mm == 30) {
    return time_point_wrap{l_y, l_m, l_d, 12, 0, l_s};
  }
  return in;
}

bool is_am_time(const time_point_wrap& in) {
  auto&& [l_y, l_m, l_d, l_h, l_mm, l_s] = in.compose();
  auto l_m1 = boost::numeric_cast<std::uint16_t>(std::round(boost::numeric_cast<std::float_t>(l_mm) / 10) * 10);
  return l_h < 12;
}

void sub_day_rest_time(const time_point_wrap& in_day, doodle::business::work_clock& in_clock) {
  auto&& [l_y, l_m, l_d, l_h, l_mm, l_s] = in_day.compose();

  in_clock -= std::make_tuple(time_point_wrap{l_y, l_m, l_d, 12}, time_point_wrap{l_y, l_m, l_d, 13});
  in_clock -= std::make_tuple(time_point_wrap{l_y, l_m, l_d, 18}, time_point_wrap{l_y, l_m, l_d, 18, 30});
}

}  // namespace

void attendance::add_clock_data(doodle::business::work_clock& in_clock) const {
  if (attendance_result_list.size() == 2) {
    /// 正常打卡, 并且审批单为空
    if (!class_setting_info_attr.rest_time_vo_list_attr.empty()) {
      auto l_t = std::make_tuple(time_point_wrap{}, time_point_wrap{});
      ranges::for_each(attendance_result_list, [&](const attendance_result& in_r) {
        switch (in_r.check_type) {
          case detail::check_type::OnDuty:
            std::get<0>(l_t) = in_r.plan_check_time;
            break;
          case detail::check_type::OffDuty:
            std::get<1>(l_t) = in_r.plan_check_time;
            break;
        }
      });
      in_clock += l_t;
      if (!class_setting_info_attr.rest_time_vo_list_attr.empty()) {
        auto l_sub_t = std::make_tuple(
            std::get<0>(l_t) + class_setting_info_attr.rest_time_vo_list_attr.front().rest_begin_time -
                chrono::hours{1},
            std::get<0>(l_t) + class_setting_info_attr.rest_time_vo_list_attr.front().rest_end_time - chrono::hours{1}
        );
        DOODLE_LOG_INFO("去除午休时间 {} -> {}", std::get<0>(l_sub_t), std::get<0>(l_sub_t));
        in_clock -= l_sub_t;
      }
    } else {
      DOODLE_LOG_INFO(" {} 中午休息为空, 认为是在节假日", work_date);
    }
    /// 有审批单, 并且没有午休, 认为是周末, 跳过

  } else if (attendance_result_list.size() == 1) {
    DOODLE_LOG_INFO("打卡列表不是两次, 跳过");
  }

  ranges::for_each(approve_list, [&](const approve_for_open& in_approve_for_open) {
    time_point_wrap l_end{};
    auto l_begin = in_approve_for_open.begin_time;

    if (in_approve_for_open.duration_unit == "HOUR") {  /// 以小时计算认为只有不到一天
      /// 如果为时间段, 我们使用特殊的方法添加时间, 主要是持续时间和信息时间不一致
      auto l_t = std::stoi(in_approve_for_open.duration);
      if (l_t > 8) {
        throw_exception(doodle_error{"错误的时间段长度"});
      }
      switch (in_approve_for_open.biz_type) {
        case detail::approve_type::leave: {  /// 请假
          /// 调休的时候的时候有的人是填的八点多到12点什么的, 需要转换为从9点开始
          l_begin = in_clock.next_point(l_begin);
          /// 八小时加班(或者调休)从上午开始必须分开为三个小时以及五个小时, 中间加上午休的一个小时
          if (is_am_time(l_begin) && l_t == 4) {  /// 开始时间是上午 4个小时需要回退到3个小时
            l_t = 3;
          }
          l_end = in_clock.next_time(l_begin, chrono::hours{l_t});
        }
        case detail::approve_type::work_overtime: {  /// 加班的时间还是可以信任的
          auto l_tmp_work = in_clock;                /// 创建临时时钟
          l_tmp_work += std::make_tuple(
              in_approve_for_open.begin_time, in_approve_for_open.end_time + chrono::hours{12}
          );  /// 将所有的加班时间全部添加到时钟中去
          sub_day_rest_time(in_approve_for_open.begin_time, l_tmp_work);  /// 添加当天的休息时间
          // auto l_t =
          //     chrono::floor<chrono::hours>(l_tmp_work(in_approve_for_open.begin_time, in_approve_for_open.end_time)
          //     );                                       /// 计算并销去多余时间
          l_end = l_tmp_work.next_time(l_begin, chrono::hours{l_t});  /// 重新计算开始时间
        }
        default:
          break;
      }

    } else if (in_approve_for_open.duration_unit == "DAY") {  /// 这里我们认为只有小数 0.5或0
      /// @warning 这个逻辑只使用于调休或者请假, 加班不行
      /// @warning 这里钉钉不知道为什么很鸡巴操蛋, 单位是 DAY 时, 也他妈返回的是 4.0 靠 然后半天还是按照 3 小时算
      /// 钉钉在半天的时候, 中午会返回13:30分, 要转换为 12点

      switch (in_approve_for_open.biz_type) {
        case detail::approve_type::leave: {
          l_begin        = time_13_30_to_12_00(l_begin);  /// 是中午的话直接返回 12点
          auto l_t       = std::stod(in_approve_for_open.duration);
          auto l_t_zheng = std::floor(l_t / 8);
          auto l_t_xiao  = (l_t / 8) - l_t_zheng;
          if (l_t_xiao != 0 && l_t_xiao != 0.5) {
            throw_exception(doodle_error{"错误的时间段长度"});
          }
          if (l_t_zheng == 0) {  /// 只有一天(小数只可能是0.5)
            l_end = in_clock.next_time(l_begin, chrono::hours{is_am_time(l_begin) ? 3 : 5});
          } else {  /// 有好几天
            auto l_hours_du = boost::numeric_cast<std::chrono::hours::rep>(l_t);
            l_end           = in_clock.next_time(l_begin, chrono::hours{l_hours_du});
          }

          break;
        }
        case detail::approve_type::work_overtime: {
          throw_exception(doodle_error{"加班超过一天, 无法计算"});
        }
        default:
          break;
      }
    }
    DOODLE_LOG_INFO("审批单时间 {} {} ", l_begin, l_end);
    switch (in_approve_for_open.biz_type) {
      case detail::approve_type::leave: {  ///  请假
        in_clock -= std::make_tuple(
            time_ounding(l_begin), time_ounding(l_end), in_approve_for_open.tag_name + in_approve_for_open.sub_type
        );
        break;
      }
      case detail::approve_type::business_travel:
        break;                                     /// 出差不管
      case detail::approve_type::work_overtime: {  /// 加班
        in_clock += std::make_tuple(
            time_ounding(l_begin), time_ounding(l_end), in_approve_for_open.tag_name + in_approve_for_open.sub_type
        );
        break;
      }
      case detail::approve_type::card:  /// 补卡, 不要管
        break;
    }
    sub_day_rest_time(l_begin, in_clock);
  });
}

void to_json(nlohmann::json& nlohmann_json_j, const attendance::rest_time_vo_list& nlohmann_json_t) {
  nlohmann_json_j["rest_begin_time"] = nlohmann_json_t.rest_begin_time;
  nlohmann_json_j["rest_end_time"]   = nlohmann_json_t.rest_end_time;
}
void from_json(const nlohmann::json& nlohmann_json_j, attendance::rest_time_vo_list& nlohmann_json_t) {
  nlohmann_json_t.rest_begin_time = chrono::milliseconds{nlohmann_json_j.at("rest_begin_time").get<std::int32_t>()};
  nlohmann_json_t.rest_end_time   = chrono::milliseconds{nlohmann_json_j.at("rest_end_time").get<std::int32_t>()};
}

void to_json(nlohmann::json& nlohmann_json_j, const attendance::class_setting_info& nlohmann_json_t) {
  nlohmann_json_j["rest_time_vo_list"] = nlohmann_json_t.rest_time_vo_list_attr;
}
void from_json(const nlohmann::json& nlohmann_json_j, attendance::class_setting_info& nlohmann_json_t) {
  nlohmann_json_j.at("rest_time_vo_list").get_to(nlohmann_json_t.rest_time_vo_list_attr);
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
  nlohmann_json_j["class_setting_info"]     = nlohmann_json_t.class_setting_info_attr;
  //  nlohmann_json_j["check_record_list"]      = nlohmann_json_t.check_record_list;
}

void from_json(const nlohmann::json& nlohmann_json_j, attendance& nlohmann_json_t) {
  nlohmann_json_t.work_date = tool::parse_dingding_time(nlohmann_json_j.at("work_date"));
  nlohmann_json_j.at("attendance_result_list").get_to(nlohmann_json_t.attendance_result_list);
  nlohmann_json_j.at("userid").get_to(nlohmann_json_t.userid);
  nlohmann_json_j.at("approve_list").get_to(nlohmann_json_t.approve_list);
  nlohmann_json_j.at("corpId").get_to(nlohmann_json_t.corpId);
  if (nlohmann_json_j.contains("class_setting_info"))
    nlohmann_json_j.at("class_setting_info").get_to(nlohmann_json_t.class_setting_info_attr);
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

}  // namespace doodle::dingding::attendance
