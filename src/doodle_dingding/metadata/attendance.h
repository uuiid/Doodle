//
// Created by TD on 2022/9/16.
//

#pragma once

#include <doodle_dingding/doodle_dingding_fwd.h>
#include <doodle_core/core/chrono_.h>
#include <doodle_core/metadata/time_point_wrap.h>
#include <nlohmann/json.hpp>

namespace doodle::dingding::attendance {

namespace query {
class DOODLE_DINGDING_API get_day_data {
 public:
  time_point_wrap work_date_from;
  time_point_wrap work_date_to;
  std::vector<std::string> user_id_list;
  int64_t offset;
  int64_t limit;
  bool is_i18_n;
  friend void to_json(nlohmann::json& nlohmann_json_j, const get_day_data& nlohmann_json_t);
  friend void from_json(const nlohmann::json& nlohmann_json_j, get_day_data& nlohmann_json_t) ;
};
};  // namespace query

class DOODLE_DINGDING_API approve_for_open {
 public:
  std::string duration_unit{};
  std::string duration{};
  std::string sub_type{};
  std::string tag_name{};
  std::string procInst_id{};
  time_point_wrap begin_time{};
  std::int32_t biz_type{};
  time_point_wrap end_time{};
  time_point_wrap gmt_finished{};

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(
      approve_for_open,
      duration_unit,
      duration,
      sub_type,
      tag_name,
      procInst_id,
      begin_time,
      biz_type,
      end_time,
      gmt_finished
  )
};

class DOODLE_DINGDING_API attendance_result {
 public:
  std::int32_t record_id{};
  std::string source_type{};
  time_point_wrap plan_check_time{};
  std::int32_t class_id{};
  std::string location_method{};
  std::string location_result{};
  std::string outside_remark{};
  std::int32_t plan_id{};
  std::string user_address{};
  std::int32_t group_id{};
  time_point_wrap user_check_time{};
  std::string procInst_id{};
  std::string check_type{};
  std::string time_result{};

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(
      attendance_result,
      record_id,
      source_type,
      plan_check_time,
      class_id,
      location_method,
      location_result,
      outside_remark,
      plan_id,
      user_address,
      group_id,
      user_check_time,
      procInst_id,
      check_type,
      time_result
  ),
};

class DOODLE_DINGDING_API attendance_record {
 public:
  std::int64_t record_id{};
  std::string source_type{};
  std::string user_accuracy{};
  bool valid_matched{};
  std::string user_check_time{};
  std::string invalid_record_msg{};
  std::string invalid_record_type{};
  //  std::string user_longitude;
  //  std::string user_latitude;
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(
      attendance_record,
      record_id,
      source_type,
      user_accuracy,
      valid_matched,
      user_check_time,
      invalid_record_msg,
      invalid_record_type
  )
};

// class DOODLE_DINGDING_API attendance {
//  public:
//   time_point_wrap date{};
//   std::vector<attendance_result> attendance_result_list{};
//   std::int32_t userid{};
//   std::vector<approve_for_open> approve_list{};
//   std::string corp_id{};
//   std::vector<attendance_record> check_record_list{};
//   NLOHMANN_DEFINE_TYPE_INTRUSIVE(
//       attendance,
//       date,
//       attendance_result_list,
//       userid,
//       approve_list,
//       corp_id,
//       check_record_list
//   )
// };

class DOODLE_DINGDING_API day_data {
 public:
  std::string check_type;
  std::string location_result;
  int64_t base_check_time;
  int64_t group_id;
  std::string time_result;
  std::string user_id;
  int64_t record_id;
  int64_t work_date;
  std::string source_type;
  int64_t user_check_time;
  int64_t plan_id;
  int64_t id;

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(
      day_data,
      check_type,
      location_result,
      base_check_time,
      group_id,
      time_result,
      user_id,
      record_id,
      work_date,
      source_type,
      user_check_time,
      plan_id,
      id
  )
};
}  // namespace doodle::dingding::attendance
