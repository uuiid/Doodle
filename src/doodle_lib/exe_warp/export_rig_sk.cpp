#include "export_rig_sk.h"

#include "doodle_core/core/core_set.h"
#include "doodle_core/core/file_sys.h"
#include "doodle_core/exception/exception.h"

#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/exe_warp/ue_exe.h>

#include <boost/asio/awaitable.hpp>

#include <filesystem>

namespace doodle {

boost::asio::awaitable<void> export_rig_sk_arg::run() const {
  auto l_arg       = std::make_shared<maya_exe_ns::export_rig_arg>();
  l_arg->file_path = maya_file_;
  auto l_maya_result = co_await async_run_maya(l_arg, logger_);
  auto l_ue_project = ue_exe_ns::find_ue_project_file(ue_path_);
  if (l_ue_project.empty()) throw doodle_error{"无法找到UE项目文件 {}", ue_path_};
  auto l_local_ue_project = core_set::get_set().get_cache_root(l_ue_project.stem());
  FSys::copy_diff(l_ue_project.parent_path(), l_local_ue_project, logger_);



  co_return;
}
}  // namespace doodle