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

  void export_fbx_file(const entt::handle& in_handle, const details::export_fbx_arg& in_arg);
  void qcloth_sim_file(const entt::handle& in_handle, const details::qcloth_arg& in_arg);
};

}  // namespace doodle
