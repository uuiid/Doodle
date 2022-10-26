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
  std::vector<std::tuple<time_point_wrap, time_point_wrap, std::string>> holidaycn_list;

 public:
  holidaycn_time();
  virtual ~holidaycn_time();

  void set_clock(const bu);
};

}  // namespace doodle
