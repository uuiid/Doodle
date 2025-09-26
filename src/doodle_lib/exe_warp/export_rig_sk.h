#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/asyn_task.h>

#include <boost/signals2.hpp>

#include <filesystem>
#include <string>

namespace doodle {
class export_rig_sk_arg : public async_task {
 private:
  mutable FSys::path result_path_;

 public:
  FSys::path maya_file_{};
  FSys::path ue_path_{};
  uuid asset_type_id_{};
  std::string bian_hao_{};
  std::string pin_yin_ming_cheng_{};
  std::string ban_ben_{};
  FSys::path project_name_{};
  boost::signals2::signal<void(const server_task_info::run_time_info_t&)> on_run_time_info_;
  // 获取结果
  FSys::path get_result() const;

  // from json
  friend void from_json(const nlohmann::json& in_json, export_rig_sk_arg& out_obj) {
    if (in_json.contains("maya_file")) in_json.at("maya_file").get_to(out_obj.maya_file_);
    if (in_json.contains("ue_path")) in_json.at("ue_path").get_to(out_obj.ue_path_);
    if (in_json.contains("asset_type_id")) in_json.at("asset_type_id").get_to(out_obj.asset_type_id_);
    if (in_json.contains("bian_hao")) in_json.at("bian_hao").get_to(out_obj.bian_hao_);
    if (in_json.contains("pin_yin_ming_cheng")) in_json.at("pin_yin_ming_cheng").get_to(out_obj.pin_yin_ming_cheng_);
    if (in_json.contains("ban_ben")) in_json.at("ban_ben").get_to(out_obj.ban_ben_);
  }
  // to json
  friend void to_json(nlohmann::json& in_json, const export_rig_sk_arg& out_obj) {
    in_json["maya_file"]          = out_obj.maya_file_;
    in_json["ue_path"]            = out_obj.ue_path_;
    in_json["asset_type_id"]      = out_obj.asset_type_id_;
    in_json["bian_hao"]           = out_obj.bian_hao_;
    in_json["pin_yin_ming_cheng"] = out_obj.pin_yin_ming_cheng_;
    in_json["ban_ben"]            = out_obj.ban_ben_;
  }

  boost::asio::awaitable<void> run() override;
};
}  // namespace doodle