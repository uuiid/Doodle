#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/core/asyn_task.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/http_client/kitsu_client.h>

#include <vector>

namespace doodle {
class update_ue_files : public async_task {
 public:
  update_ue_files()          = default;
  virtual ~update_ue_files() = default;

  std::vector<uuid> task_ids_{};
  std::shared_ptr<kitsu::kitsu_client> kitsu_client_{};

  struct args {
    std::vector<FSys::path> ue_file_list_{};
    // from json
    friend void from_json(const nlohmann::json& in_json, update_ue_files::args& out_obj) {
      in_json.at("ue_file_list").get_to(out_obj.ue_file_list_);
    }
    // to json
    friend void to_json(nlohmann::json& out_json, const update_ue_files::args& in_obj) {
      out_json["ue_file_list"] = in_obj.ue_file_list_;
    }
  };

  boost::asio::awaitable<void> run() override;
};
}  // namespace doodle