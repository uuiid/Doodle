#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/lib_warp/json_warp.h>

#include <boost/signals2.hpp>
namespace doodle {
namespace details {
class qcloth_arg;
class export_fbx_arg;
}  // namespace details

class DOODLELIB_API maya_file_async : public details::no_copy {
 public:
  maya_file_async();

  void export_fbx_file(const std::vector<FSys::path>& in_vector,
                       const details::export_fbx_arg& in_arg,
                       const std::int32_t in_maz_ther);
  /**
   * @brief 这个是一起进行批次解算
   * @param in_handle 传入的解算文件
   * @param in_arg 传入的解算参数
   */
  void qcloth_sim_file(const std::vector<FSys::path>& in_vector,
                       const details::qcloth_arg& in_arg,
                       const std::int32_t in_maz_ther);
};

}  // namespace doodle
