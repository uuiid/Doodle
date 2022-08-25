//
// Created by TD on 2021/12/27.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {

class DOODLELIB_API move_attr {
 public:
  /**
   * @brief 这个是输出的文件夹名称
   */
  FSys::path out_dir;

  inline bool operator<(const move_attr &in_rhs) const {
    return out_dir < in_rhs.out_dir;
  }
  inline bool operator>(const move_attr &in_rhs) const {
    return in_rhs < *this;
  }
  inline bool operator<=(const move_attr &in_rhs) const {
    return !(in_rhs < *this);
  }
  inline bool operator>=(const move_attr &in_rhs) const {
    return !(*this < in_rhs);
  }
};

namespace details {

/**
 * @brief 连接视频实现
 */
class DOODLELIB_API join_move : public process_t<join_move> {
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  using base_type = process_t<join_move>;
  /**
   * @brief 将传入的视频序列连接为一个视频
   * @param in_handle 具有消息组件, 和 *输出路径文件夹* 组件的的句柄
   * @param in_vector 视频序列的句柄, 不需要排序, 会根据名称自动排序
   */
  join_move(const entt::handle &in_handle, const std::vector<FSys::path> &in_vector);
  virtual ~join_move();
  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(const base_type::delta_type &, void *data);
  void link_move();
};
}  // namespace details

}  // namespace doodle
