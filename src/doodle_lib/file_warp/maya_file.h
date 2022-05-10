#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_core/lib_warp/json_warp.h>

#include <boost/signals2.hpp>
namespace doodle {
namespace details {
class qcloth_arg;
class export_fbx_arg;
class replace_file_arg;

}  // namespace details

class DOODLELIB_API maya_file_async : public details::no_copy {
 public:
  maya_file_async();

  void export_fbx_file(const entt::handle& in_handle, const details::export_fbx_arg& in_arg);

  /**
   * @brief 这个是一起进行批次解算
   * @param in_handle 传入的解算文件
   * @param in_arg 传入的解算参数
   */
  void qcloth_sim_file(const entt::handle& in_handle, const details::qcloth_arg& in_arg);
  void replace_file_fun(const entt::handle& in_handle, const details::replace_file_arg& in_arg);
};

}  // namespace doodle
