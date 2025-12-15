#pragma once

#include "doodle_core/core/file_sys.h"
#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/metadata/project.h"
#include <doodle_core/core/http_client_core.h>
#include <doodle_core/metadata/assets.h>

#include <doodle_lib/core/http/json_body.h>

#include <boost/algorithm/string.hpp>
#include <boost/asio/awaitable.hpp>

#include <filesystem>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <tl/expected.hpp>
#include <vector>

namespace doodle {
class async_task;
}

namespace doodle::kitsu {

class kitsu_client {
  using http_client_t     = doodle::http::http_client;
  using http_client_ptr_t = std::shared_ptr<http_client_t>;
  http_client_ptr_t http_client_ptr_{};
  std::string kitsu_token_;

  boost::asio::awaitable<void> upload_asset_file(
      std::string in_upload_url, FSys::path in_file_path, std::string in_file_field_name
  ) const;
  boost::asio::awaitable<void> remove_asset_file(std::string in_upload_url) const;
  template <typename T>
  void set_req_headers(
      boost::beast::http::request<T>& req, const std::string& in_content_type = "application/json"
  ) const;

  logger_ptr logger_{spdlog::default_logger()};

 public:
  explicit kitsu_client(const std::string& kitsu_url) : http_client_ptr_{std::make_shared<http_client_t>(kitsu_url)} {}
  template <typename ExecutorType>
  explicit kitsu_client(const std::string& kitsu_url, ExecutorType&& in_executor)
      : http_client_ptr_{std::make_shared<http_client_t>(kitsu_url, std::forward<ExecutorType>(in_executor))} {}
  struct file_association {
    FSys::path ue_file_;
    FSys::path maya_file_;
    details::assets_type_enum type_;
  };
  struct update_file_arg {
    FSys::path local_path_;
    std::string field_name_;

    static std::vector<update_file_arg> list_all_project_files(
        const FSys::path& in_project_path, const std::vector<FSys::path>& in_extra_path = {}
    );
  };
  void set_logger(logger_ptr in_logger) { logger_ = std::move(in_logger); }

  // set token
  void set_token(const std::string& in_token) { kitsu_token_ = in_token; }
  // get token
  const std::string& get_token() const { return kitsu_token_; }

  boost::asio::awaitable<file_association> get_file_association(uuid in_task_id) const;

  boost::asio::awaitable<FSys::path> get_ue_plugin(std::string in_version) const;

  boost::asio::awaitable<FSys::path> get_task_maya_file(uuid in_task_id) const;

  boost::asio::awaitable<project> get_project(uuid in_project_id) const;
  boost::asio::awaitable<nlohmann::json> get_generate_uesk_file_arg(uuid in_task_id) const;
  boost::asio::awaitable<void> upload_asset_file_maya(uuid in_task_id, FSys::path in_file_path) const;
  /// 上传UE文件
  boost::asio::awaitable<void> upload_asset_file_ue(uuid in_task_id, std::vector<update_file_arg> in_file_path) const;
  boost::asio::awaitable<void> upload_asset_file_image(uuid in_task_id, FSys::path in_file_path) const;
  /// 上传镜头maya文件
  boost::asio::awaitable<void> upload_shot_animation_maya(uuid in_shot_task_id, FSys::path in_file_path);
  /// 上传镜头导出文件
  boost::asio::awaitable<void> upload_shot_animation_export_file(
      uuid in_shot_task_id, FSys::path in_dir, FSys::path in_file_name
  );
  /// 上传镜头其他文件
  boost::asio::awaitable<void> upload_shot_animation_other_file(
      uuid in_shot_task_id, FSys::path in_dir, FSys::path in_file_name
  );
  /// 上传镜头UE文件
  boost::asio::awaitable<void> upload_shot_animation_ue(
      uuid in_shot_task_id, std::vector<update_file_arg> in_file_path
  ) const;
  boost::asio::awaitable<void> remove_asset_file_maya(const uuid& in_uuid);
  boost::asio::awaitable<void> remove_asset_file_ue(const uuid& in_uuid);
  boost::asio::awaitable<void> remove_asset_file_image(const uuid& in_uuid);
  boost::asio::awaitable<void> remove_shot_animation_maya(const uuid& in_uuid);
  boost::asio::awaitable<void> remove_shot_animation_export_file(const uuid& in_uuid);
  boost::asio::awaitable<void> remove_shot_animation_other_file(const uuid& in_uuid);
  boost::asio::awaitable<nlohmann::json> get_ue_assembly(uuid in_project_id, uuid in_shot_task_id) const;
  /// 对task进行评论(并且附加预览图或者视频)
  boost::asio::awaitable<void> comment_task(
      uuid in_task_id, const std::string& in_comment, const FSys::path& in_attach_files,
      const uuid& in_task_status_id = uuid{}, const std::vector<std::string>& in_checklists = {},
      const std::vector<std::string>& in_links = {}
  ) const;
  boost::asio::awaitable<nlohmann::json> get_export_anim_fbx(uuid in_task_id) const;
  boost::asio::awaitable<nlohmann::json> get_task_sync(uuid in_task_id) const;
};

}  // namespace doodle::kitsu