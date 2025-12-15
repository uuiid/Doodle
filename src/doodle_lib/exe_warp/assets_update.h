#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/core/asyn_task.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/http_client/kitsu_client.h>

#include <filesystem>
#include <vector>

namespace doodle {
class update_ue_files : public async_task {
 public:
  update_ue_files()          = default;
  virtual ~update_ue_files() = default;

  uuid task_id_{};
  std::shared_ptr<kitsu::kitsu_client> kitsu_client_{};

  FSys::path ue_project_path_{};

  boost::asio::awaitable<void> run() override;
};

class update_image_files : public async_task {
 public:
  update_image_files()          = default;
  virtual ~update_image_files() = default;

  uuid task_id_{};
  std::shared_ptr<kitsu::kitsu_client> kitsu_client_{};

  std::vector<FSys::path> image_files_{};

  boost::asio::awaitable<void> run() override;
};

}  // namespace doodle