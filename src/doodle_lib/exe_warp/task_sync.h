#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include "doodle_lib/core/asyn_task.h"
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/http_client/kitsu_client.h>

#include <vector>

namespace doodle {
class DOODLELIB_API task_sync : public async_task {
 public:
  task_sync()          = default;
  virtual ~task_sync() = default;

  std::vector<uuid> task_ids_{};
  std::shared_ptr<kitsu::kitsu_client> kitsu_client_{};

  boost::asio::awaitable<void> run() override;
};
}  // namespace doodle