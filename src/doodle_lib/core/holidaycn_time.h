//
// Created by TD on 2022/10/26.
//
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
/**
 * 这个类是一个静态嵌入文件的包装, 文件是嵌入的国务院发布的每年的法定节假日
 */
class holidaycn_time2 {
 public:
  using duration_type        = chrono::seconds;
  using time_duration_vector = std::vector<std::pair<duration_type, duration_type>>;
  class info {
   public:
    std::string name;
    chrono::local_days date;
    bool is_odd_day;  // 是放假

    friend void to_json(nlohmann::json &in_j, const info &in_p);
    friend void from_json(const nlohmann::json &in_j, info &in_p);
  };

 private:
  std::vector<std::tuple<chrono::local_days, chrono::local_days, std::string>> holidaycn_list_rest;
  std::vector<std::tuple<chrono::local_time_pos, chrono::local_time_pos, std::string>> holidaycn_list_work;

  void load_year(const FSys::path &in_path);
  time_duration_vector work_time{};
  std::map<chrono::local_days, info> work_day_map{};

 public:
  explicit holidaycn_time2(time_duration_vector in_work_time, const FSys::path &in_path);
  virtual ~holidaycn_time2();

  void set_clock(business::work_clock2 &in_work_clock) const;
  bool is_working_day(const chrono::year_month_day &in_day) const { return is_working_day(chrono::local_days{in_day}); }
  bool is_working_day(const chrono::local_days &in_day) const;
};

}  // namespace doodle
