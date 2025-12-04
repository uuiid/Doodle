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

  bool update_{true};
  bool download_{true};

  struct copy_info {
    FSys::path from_path_{};
    FSys::path to_path_{};

    // 此处记录一下文件对对应的task id , 这个是上传时专用的, 下载时忽略
    uuid task_id_{};

    // to json
    friend void to_json(nlohmann::json& out_json, const copy_info& in_obj) {
      out_json["from_path"] = in_obj.from_path_;
      out_json["to_path"]   = in_obj.to_path_;
    }
    // from json
    friend void from_json(const nlohmann::json& in_json, copy_info& out_obj) {
      in_json.at("from_path").get_to(out_obj.from_path_);
      in_json.at("to_path").get_to(out_obj.to_path_);
    }
    // equal
    bool operator==(const copy_info& in_other) const {
      return from_path_ == in_other.from_path_ && to_path_ == in_other.to_path_;
    }
  };

  struct args {
    std::vector<copy_info> update_file_list_{};
    std::vector<copy_info> download_file_list_{};
    friend void from_json(const nlohmann::json& in_json, task_sync::args& out_obj) {
      in_json.at("update_file_list").get_to(out_obj.update_file_list_);
      in_json.at("download_file_list").get_to(out_obj.download_file_list_);
    }
    friend void to_json(nlohmann::json& out_json, const task_sync::args& in_obj) {
      out_json["update_file_list"]   = in_obj.update_file_list_;
      out_json["download_file_list"] = in_obj.download_file_list_;
    }

    args& operator+=(const args& in_other);
  };

  // from json
  friend void from_json(const nlohmann::json& in_json, task_sync& out_obj) {
    in_json.at("task_ids").get_to(out_obj.task_ids_);
    in_json.at("update").get_to(out_obj.update_);
    in_json.at("download").get_to(out_obj.download_);
  }
  // to json
  friend void to_json(nlohmann::json& out_json, const task_sync& in_obj) {
    out_json["task_ids"] = in_obj.task_ids_;
    out_json["update"]   = in_obj.update_;
    out_json["download"] = in_obj.download_;
  }

  boost::asio::awaitable<void> run() override;
};
}  // namespace doodle