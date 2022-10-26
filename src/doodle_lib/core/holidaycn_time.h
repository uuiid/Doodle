//
// Created by TD on 2022/10/26.
//
#include <doodle_lib_fwd.h>

namespace doodle {
/**
 * 这个类是一个静态嵌入文件的包装, 文件是嵌入的国务院发布的每年的法定节假日
 * 现在只有 2022 年的信息
 */
class holidaycn_time {
 private:
  std::vector<std::tuple<time_point_wrap, time_point_wrap, std::string>> holidaycn_list_rest;
  std::vector<std::tuple<time_point_wrap, time_point_wrap, std::string>> holidaycn_list_work;

 public:
  class info {
   public:
    std::string name;
    chrono::local_days date;
    bool is_odd_day;

    friend void to_json(nlohmann::json &in_j, const info &in_p);
    friend void from_json(const nlohmann::json &in_j, info &in_p);
  };

  holidaycn_time();
  virtual ~holidaycn_time();

  void set_clock(business::work_clock &in_work_clock) const;
};

}  // namespace doodle
