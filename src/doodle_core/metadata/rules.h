//
// Created by TD on 2022/8/4.
//
#pragma once

#include <doodle_core/doodle_core.h>
#include <doodle_core/metadata/detail/time_point_info.h>

#include <bitset>
#include <chrono>
#include <utility>

namespace doodle {
class time_point_wrap;
}

namespace doodle::business {
class rules;
void to_json(nlohmann::json& j, const rules& p);
void from_json(const nlohmann::json& j, rules& p);

namespace rules_ns {
class time_point_info;
};

/**
 * @brief 这个时间规则是一个本地时间(并非 utc 时间)
 */

class DOODLE_CORE_API rules {
 public:
  using duration_type        = chrono::seconds;
  using point_type           = rules_ns::time_point_info;

  using time_duration_vector = std::vector<std::pair<duration_type, duration_type>>;
  /// \brief 周六 ->周日(index 6->0)
  using work_day_type        = std::bitset<7>;

 private:
  friend void to_json(nlohmann::json& j, const rules& p);
  friend void from_json(const nlohmann::json& j, rules& p);

 public:
  /// \brief 周六 ->周日(index 6->0)
  constexpr static work_day_type work_Monday_to_Friday{0b0111110};
  /// @brief 周六到周日每日必然会排除的时间

  using time_point_info = ::doodle::business::rules_ns::time_point_info;

  work_day_type work_weekdays_p{};
  time_duration_vector work_pair_p{};
  std::array<time_duration_vector, 7> absolute_deduction{};

  std::vector<time_point_info> extra_p{};
  rules();
  virtual ~rules();

  rules(const rules& in_rules) noexcept            = default;
  rules& operator=(const rules& in_rules) noexcept = default;
  rules(rules&& in_rules) noexcept                 = default;
  rules& operator=(rules&& in_rules) noexcept      = default;
  /**
   * @brief 获取每周 1-5工作, 每天9-12 13-18 工作时间的默认时间段
   * @return 默认段规则
   */
  [[nodiscard("")]] static const rules& get_default();

  void work_weekdays(const work_day_type& in_work_weekdays);
  [[nodiscard("")]] const work_day_type& work_weekdays() const;
  [[nodiscard("")]] work_day_type& work_weekdays();

 private:
  friend struct ::fmt::formatter<::doodle::business::rules>;
  [[nodiscard]] std::string fmt_str() const;
};

}  // namespace doodle::business
namespace fmt {
/**
 * @brief 季数格式化程序
 *
 * @tparam
 */
template <>
struct formatter<::doodle::business::rules> : formatter<string_view> {
  template <typename FormatContext>
  auto format(const ::doodle::business::rules& in_, FormatContext& ctx) const -> decltype(ctx.out()) {
    return formatter<string_view>::format(in_.fmt_str(), ctx);
  }
};
}  // namespace fmt
