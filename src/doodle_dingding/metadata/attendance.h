//
// Created by TD on 2022/9/16.
//

#pragma once

#include <doodle_core/core/chrono_.h>
#include <doodle_core/lib_warp/std_fmt_optional.h>
#include <doodle_core/metadata/time_point_wrap.h>

#include <doodle_dingding/doodle_dingding_fwd.h>
#include <nlohmann/json.hpp>

namespace doodle::business {
class work_clock;
}

namespace doodle::dingding::attendance {

namespace query {
class DOODLE_DINGDING_API get_day_data {
 public:
  time_point_wrap workDateFrom{};
  time_point_wrap workDateTo{};
  std::vector<std::string> userIdList{};
  int64_t offset{};
  int64_t limit{};
  bool isI18n{};
  friend void DOODLE_DINGDING_API to_json(nlohmann::json& nlohmann_json_j, const get_day_data& nlohmann_json_t);
  friend void DOODLE_DINGDING_API from_json(const nlohmann::json& nlohmann_json_j, get_day_data& nlohmann_json_t);
};
class DOODLE_DINGDING_API get_update_data {
 public:
  time_point_wrap work_date{};
  std::string userid{};
  friend void DOODLE_DINGDING_API to_json(nlohmann::json& nlohmann_json_j, const get_update_data& nlohmann_json_t);
  friend void DOODLE_DINGDING_API from_json(const nlohmann::json& nlohmann_json_j, get_update_data& nlohmann_json_t);
};

};  // namespace query
namespace detail {
enum class source_type : std::uint16_t {
  ATM,        /// @brief 考勤机
  BEACON,     /// @brief IBeacon(不知道什么东西)
  DING_ATM,   /// @brief 钉钉考勤机
  USER,       /// @brief 用户打卡
  BOSS,       /// @brief 老板 改签
  APPROVE,    /// @brief 审批系统
  SYSTEM,     /// @brief 考勤系统
  AUTO_CHECK  /// @brief 自动打卡
};

enum class location_result : std::uint16_t {
  Normal,    /// @brief 范围内(正常)
  Outside,   /// @brief 范围外
  NotSigned  /// @brief 未打卡
};
enum class time_result : std::uint16_t {
  Normal,       /// @brief 正常
  Early,        /// @brief 早退
  Late,         /// @brief 迟到
  SeriousLate,  /// @brief 严重迟到
  Absenteeism,  /// @brief 旷工迟到
  NotSigned     /// @brief 未打卡
};
enum class check_type : std::uint16_t {
  OnDuty,   /// @brief 上班
  OffDuty,  /// @brief下班
};

enum class approve_type : std::uint16_t {
  leave,            /// @brief 请假
  business_travel,  /// @brief 出差
  work_overtime     /// @brief 加班
};

}  // namespace detail

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
      attendance_record, record_id, source_type, user_accuracy, valid_matched, user_check_time, invalid_record_msg,
      invalid_record_type
  )
};

class DOODLE_DINGDING_API attendance {
 public:
  class DOODLE_DINGDING_API attendance_result {
   public:
    /// @brief 打卡流水id
    std::int32_t record_id{};
    /// @brief 来源类型
    detail::source_type source_type{};
    /// @brief 标准打卡时间
    time_point_wrap plan_check_time{};
    /// @brief 班次id
    std::int32_t class_id{};
    /// @brief 定位方法
    std::string location_method{};
    /// @brief 定位结果
    detail::location_result location_result{};
    /// @brief 外勤备注
    std::string outside_remark{};
    /// @brief 排班id
    std::int32_t plan_id{};
    /// @brief 用户打卡地址
    std::string user_address{};
    /// @brief 考勤组id
    std::int32_t group_id{};
    /// @brief 用户打卡时间
    time_point_wrap user_check_time{};
    /// @brief 审批单id
    std::string procInst_id{};
    /// @brief 打卡类型
    detail::check_type check_type{};
    /// @brief 打卡结果
    detail::time_result time_result{};
    friend void DOODLE_DINGDING_API
    to_json(nlohmann::json& nlohmann_json_j, const attendance::attendance_result& nlohmann_json_t);
    friend void DOODLE_DINGDING_API
    from_json(const nlohmann::json& nlohmann_json_j, attendance::attendance_result& nlohmann_json_t);
  };

  class DOODLE_DINGDING_API approve_for_open {
   public:
    /// @brief 审批单的单位 (天/小时)
    std::string duration_unit{};
    /// @brief 时长 (2.0)
    std::string duration{};
    /// @brief 子类型名称 比如年假
    std::string sub_type{};
    /// @brief 标签名称
    std::string tag_name{};
    /// @brief 审批单id
    std::string procInst_id{};
    /// @brief 审批开始时间
    time_point_wrap begin_time{};
    /// @brief 审批单类型
    detail::approve_type biz_type{};
    /// @brief 审批结束时间
    time_point_wrap end_time{};
    /// @brief 审批单审批完成时间
    time_point_wrap gmt_finished{};

    friend void DOODLE_DINGDING_API
    to_json(nlohmann::json& nlohmann_json_j, const attendance::approve_for_open& nlohmann_json_t);
    friend void DOODLE_DINGDING_API
    from_json(const nlohmann::json& nlohmann_json_j, attendance::approve_for_open& nlohmann_json_t);
  };
  /// @brief 查询日期
  time_point_wrap work_date{};
  /// @brief 打卡结果
  std::vector<attendance_result> attendance_result_list{};
  /// @brief 用户id
  std::string userid{};
  /// @brief 审批单列表
  std::vector<approve_for_open> approve_list{};
  /// @brief 打卡详情列表(此处暂时没有获取)
  //  std::vector<attendance_record> check_record_list{};
  ///  @brief 企业di
  std::string corpId{};

  void add_clock_data(doodle::business::work_clock& in_clock) const;
  //  std::vector<std::pair<time_point_wrap,
  //                        time_point_wrap>>
  //      class_setting_info;  /// @brief 当前排班对应的休息时间段 -> 班次内休息信息
  friend void DOODLE_DINGDING_API to_json(nlohmann::json& nlohmann_json_j, const attendance& nlohmann_json_t);
  friend void DOODLE_DINGDING_API from_json(const nlohmann::json& nlohmann_json_j, attendance& nlohmann_json_t);
};

class DOODLE_DINGDING_API day_data {
 public:
  /// @brief 来源类型
  std::string sourceType{};
  /// @brief 基准时间(标准上下班时间)
  time_point_wrap baseCheckTime{};
  /// @brief 用户打卡时间
  time_point_wrap userCheckTime{};

  /// @brief 关联的审批实例 id
  std::optional<std::string> procInstId{};
  /// @brief 关联的审批ID
  std::optional<std::string> approveId{};

  /// @brief 位置结果 enum { Normal, Outside, NotSigned }
  std::string locationResult{};
  /// @brief 打卡结果：enum { Normal, Early, Late, SeriousLate, Absenteeism, NotSigned }
  std::string timeResult{};
  /// @brief 考勤类型: enum { OnDuty, OffDuty }
  std::string checkType{};
  /// @brief 关联的用户id
  std::string userId{};
  /// @brief 工作日
  time_point_wrap workDate{};
  /// @brief 打卡记录ID
  std::int64_t recordId{};
  /// @brief 排班ID
  std::int64_t planId{};
  /// @brief 考勤组ID
  std::int64_t groupId{};
  /// @brief 唯一标识ID
  std::int64_t id{};

  friend void DOODLE_DINGDING_API to_json(nlohmann::json& nlohmann_json_j, const day_data& nlohmann_json_t);
  friend void DOODLE_DINGDING_API from_json(const nlohmann::json& nlohmann_json_j, day_data& nlohmann_json_t);
};
}  // namespace doodle::dingding::attendance

namespace fmt {
template <>
struct formatter<::doodle::dingding::attendance::day_data> : formatter<string_view> {
  template <typename FormatContext>
  auto format(const ::doodle::dingding::attendance::day_data& in_, FormatContext& ctx) const -> decltype(ctx.out()) {
    return fmt::format_to(
        ctx.out(),
        "sourceType {}"
        " baseCheckTime {}"
        " userCheckTime {}"
        " procInstId {}"
        " approveId {}"
        " locationResult {}"
        " timeResult {}"
        " checkType {}"
        " userId {}"
        " workDate {}"
        " recordId {}"
        " planId {}"
        " groupId {}"
        " id {}",
        in_.sourceType, in_.baseCheckTime, in_.userCheckTime, in_.procInstId, in_.approveId, in_.locationResult,
        in_.timeResult, in_.checkType, in_.userId, in_.workDate, in_.recordId, in_.planId, in_.groupId, in_.id
    );
  }
};
}  // namespace fmt
