#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/asyn_task.h>
#include <doodle_lib/http_client/kitsu_client.h>

#include <boost/signals2.hpp>

#include <filesystem>
#include <string>

namespace doodle {
class export_rig_sk_arg : public async_task {
 private:
 public:
  struct file_copy_info {
    FSys::path from_;
    FSys::path to_;
    // to json
    friend void to_json(nlohmann::json& j, const file_copy_info& p) {
      j["from"] = p.from_;
      j["to"]   = p.to_;
    }
    // from json
    friend void from_json(const nlohmann::json& j, file_copy_info& p) {
      j.at("from").get_to(p.from_);
      j.at("to").get_to(p.to_);
    }
  };
  struct data_t {
    FSys::path import_game_path_{};
    FSys::path update_ue_path_{};
    FSys::path ue_project_path_{};
    std::vector<file_copy_info> ue_asset_copy_path_;  // 需要复制的UE路径
                                                      // to json
    friend void to_json(nlohmann::json& j, const data_t& p) {
      j["import_game_path"]   = p.import_game_path_;
      j["update_ue_path"]     = p.update_ue_path_;
      j["ue_project_path"]    = p.ue_project_path_;
      j["ue_asset_copy_path"] = p.ue_asset_copy_path_;
    }
    // from json
    friend void from_json(const nlohmann::json& j, data_t& p) {
      if (j.contains("import_game_path")) j.at("import_game_path").get_to(p.import_game_path_);
      if (j.contains("update_ue_path")) j.at("update_ue_path").get_to(p.update_ue_path_);
      if (j.contains("ue_project_path")) j.at("ue_project_path").get_to(p.ue_project_path_);
      if (j.contains("ue_asset_copy_path")) j.at("ue_asset_copy_path").get_to(p.ue_asset_copy_path_);
    }
  };
  data_t impl_{};
  FSys::path maya_file_{};
  uuid task_id_{};
  std::shared_ptr<kitsu::kitsu_client> kitsu_client_{};

  boost::signals2::signal<void(const server_task_info::run_time_info_t&)> on_run_time_info_;

  // from json
  friend void from_json(const nlohmann::json& in_json, export_rig_sk_arg& out_obj) {
    in_json.get_to(out_obj.impl_);
    if (in_json.contains("path")) in_json.at("path").get_to(out_obj.maya_file_);
  }
  // to json
  friend void to_json(nlohmann::json& in_json, const export_rig_sk_arg& out_obj) {
    in_json              = out_obj.impl_;
    in_json["path"] = out_obj.maya_file_;
  }

  boost::asio::awaitable<void> run() override;
};
}  // namespace doodle