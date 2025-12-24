#include "kitsu_client.h"

#include "doodle_core/configure/config.h"
#include "doodle_core/configure/static_value.h"
#include "doodle_core/core/core_set.h"
#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/exception/exception.h"
#include "doodle_core/metadata/project.h"
#include "doodle_core/metadata/task_status.h"
#include "doodle_core/metadata/task_type.h"
#include "doodle_core/metadata/working_file.h"

#include <doodle_lib/exe_warp/export_rig_sk.h>
#include <doodle_lib/exe_warp/import_and_render_ue.h>
#include <doodle_lib/exe_warp/ue_exe.h>
#include <doodle_lib/http_method/kitsu.h>

#include <boost/asio/awaitable.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/file_body_fwd.hpp>
#include <boost/beast/http/string_body_fwd.hpp>
#include <boost/scope/scope_exit.hpp>

#include "core/http/json_body.h"
#include <chrono>
#include <cpp-base64/base64.h>
#include <filesystem>
#include <fmt/compile.h>
#include <fmt/format.h>
#include <nlohmann/json_fwd.hpp>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

namespace doodle::kitsu {

// from json kitsu_client::file_association
void from_json(const nlohmann::json& j, kitsu_client::file_association& fa) {
  j.at("ue_file").get_to(fa.ue_file_);
  j.at("solve_file_").get_to(fa.maya_file_);
  j.at("type").get_to(fa.type_);
}

template <typename T>
void kitsu_client::set_req_headers(boost::beast::http::request<T>& req, const std::string& in_content_type) const {
  req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  req.set(boost::beast::http::field::accept, "application/json");
  req.set(boost::beast::http::field::host, http_client_ptr_->server_ip_and_port_);
  if (!kitsu_token_.empty()) req.set(boost::beast::http::field::authorization, fmt::format("Bearer {}", kitsu_token_));
  if (!in_content_type.empty() && req.method() != boost::beast::http::verb::get &&
      req.method() != boost::beast::http::verb::options)
    req.set(boost::beast::http::field::content_type, in_content_type);
}

boost::asio::awaitable<kitsu_client::file_association> kitsu_client::get_file_association(uuid in_task_id) const {
  boost::beast::http::request<boost::beast::http::empty_body> l_req{
      boost::beast::http::verb::get, fmt::format("/api/doodle/file_association/{}", in_task_id), 11
  };
  set_req_headers(l_req);
  boost::beast::http::response<http::basic_json_body> l_res{};
  co_await http_client_ptr_->read_and_write(l_req, l_res, boost::asio::use_awaitable);
  if (l_res.result() != boost::beast::http::status::ok)
    throw_exception(doodle_error{"kitsu get file association error"});
  co_return l_res.body().get<file_association>();
}

boost::asio::awaitable<uuid> kitsu_client::create_comment(
    uuid in_task_id, const std::string& in_comment, const uuid& in_task_status_id,
    const std::vector<std::string>& in_checklists, const std::vector<std::string>& in_links
) const {
  if (kitsu_token_.empty()) throw_exception(doodle_error{"kitsu token is empty, can not comment task"});
  uuid l_comment_id{};
  {
    nlohmann::json l_json{};
    l_json["comment"] = in_comment;
    if (!in_task_status_id.is_nil()) l_json["task_status_id"] = in_task_status_id;
    l_json["checklists"] = in_checklists;
    boost::beast::http::request<boost::beast::http::string_body> l_req{
        boost::beast::http::verb::post, fmt::format("/api/actions/tasks/{}/comment", in_task_id), 11
    };
    set_req_headers(l_req, "application/json");
    l_req.body() = l_json.dump();
    boost::beast::http::response<http::basic_json_body> l_res{};
    co_await http_client_ptr_->read_and_write(l_req, l_res, boost::asio::use_awaitable);
    if (l_res.result() != boost::beast::http::status::ok && l_res.result() != boost::beast::http::status::created)
      throw_exception(doodle_error{"kitsu comment task error {} {}", l_res.result(), l_res.body().dump()});
    l_comment_id = l_res.body().at("id").get<uuid>();
  }
  co_return l_comment_id;
}

boost::asio::awaitable<uuid> kitsu_client::create_preview(
    uuid in_task_id, uuid in_comment_id, preview_file_source_enum in_preview_file_source
) const {
  if (kitsu_token_.empty()) throw_exception(doodle_error{"kitsu token is empty, can not create preview"});
  uuid l_preview_id{};
  {  // 创建预览
    boost::beast::http::request<boost::beast::http::string_body> l_req{
        boost::beast::http::verb::post,
        fmt::format("/api/actions/tasks/{}/comments/{}/add-preview", in_task_id, in_comment_id), 11
    };
    set_req_headers(l_req, "application/json");
    l_req.body() = nlohmann::json{{"source", in_preview_file_source}}.dump();
    boost::beast::http::response<http::basic_json_body> l_res{};
    co_await http_client_ptr_->read_and_write(l_req, l_res, boost::asio::use_awaitable);
    if (l_res.result() != boost::beast::http::status::ok && l_res.result() != boost::beast::http::status::created)
      throw_exception(doodle_error{"kitsu comment task add preview error {} {}", l_res.result(), l_res.body().dump()});
    l_preview_id = l_res.body().at("id").get<uuid>();
  }
  co_return l_preview_id;
}

boost::asio::awaitable<FSys::path> kitsu_client::get_ue_plugin(std::string in_version) const {
  auto l_file_name = fmt::format("Doodle_{}.{}.zip", version::build_info::get().version_str, in_version);
  auto l_mp_name   = fmt::format("{}.zip", core_set::get_set().get_uuid());
  auto l_temp_path = core_set::get_set().get_cache_root("ue_plugin") / l_mp_name;

  boost::beast::http::request<boost::beast::http::empty_body> l_req{
      boost::beast::http::verb::get, fmt::format("/Plugins/{}", l_file_name), 11
  };
  l_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  l_req.set(boost::beast::http::field::accept, "*/*");
  l_req.set(boost::beast::http::field::host, http_client_ptr_->server_ip_and_port_);
  if (!kitsu_token_.empty())
    l_req.set(boost::beast::http::field::authorization, fmt::format("Bearer {}", kitsu_token_));
  boost::beast::http::response<boost::beast::http::file_body> l_res{};
  boost::system::error_code l_ec{};
  l_res.body().open(l_temp_path.string().c_str(), boost::beast::file_mode::write, l_ec);
  auto l_colse_and_remove_file = [&]() {
    l_res.body().close();
    if (FSys::exists(l_temp_path)) FSys::remove(l_temp_path);
  };

  if (l_ec) l_colse_and_remove_file(), throw_exception(doodle_error{"kitsu get ue plugin open file error"});
  if (!l_res.body().is_open())
    l_colse_and_remove_file(), throw_exception(doodle_error{"kitsu get ue plugin open file error"});
  boost::scope::scope_exit l_exit{[&]() {
    http_client_ptr_->body_limit_.reset();
    http_client_ptr_->set_timeout(30s);
  }};
  http_client_ptr_->body_limit_ = 100ll * 1024 * 1024 * 1024;  // 100G
  http_client_ptr_->set_timeout(1000s);
  co_await http_client_ptr_->read_and_write(l_req, l_res, boost::asio::use_awaitable);

  if (l_res.result() != boost::beast::http::status::ok)
    l_colse_and_remove_file(), throw_exception(doodle_error{"kitsu get ue plugin error"});
  co_return l_temp_path;
}

boost::asio::awaitable<FSys::path> kitsu_client::get_task_maya_file(uuid in_task_id) const {
  project l_prj{};
  std::vector<working_file> l_list{};
  {
    nlohmann::json l_json_task_full{};
    boost::beast::http::request<boost::beast::http::empty_body> l_req{
        boost::beast::http::verb::get, fmt::format("/api/data/tasks/{}/full", in_task_id), 11
    };
    set_req_headers(l_req);

    boost::beast::http::response<http::basic_json_body> l_res{};
    co_await http_client_ptr_->read_and_write(l_req, l_res, boost::asio::use_awaitable);
    if (l_res.result() != boost::beast::http::status::ok)
      throw_exception(doodle_error{"kitsu get task error {} {}", l_res.result(), l_res.body().dump()});
    l_json_task_full          = l_res.body().get<nlohmann::json>();
    const auto l_task_type_id = l_json_task_full.at("task_type_id").get<uuid>();
    l_prj                     = l_json_task_full.at("project").get<project>();
    l_list                    = l_json_task_full.at("working_files").get<std::vector<working_file>>();
  }

  auto l_it = ranges::find_if(l_list, [&](const working_file& i) { return i.software_type_ == software_enum::maya; });
  if (l_it == l_list.end()) throw_exception(doodle_error{"没有找到对应的maya working file"});
  auto l_path = l_prj.path_ / l_it->path_;
  if (l_it->path_.empty() || !FSys::exists(l_path))
    throw_exception(doodle_error{"maya working file 文件不存在 {}", l_path});

  co_return l_path;
}
boost::asio::awaitable<nlohmann::json> kitsu_client::get_generate_uesk_file_arg(uuid in_task_id) const {
  boost::beast::http::request<boost::beast::http::empty_body> l_req{
      boost::beast::http::verb::get, fmt::format("/api/actions/tasks/{}/export-rig-sk", in_task_id), 11
  };
  set_req_headers(l_req);

  boost::beast::http::response<http::basic_json_body> l_res{};
  co_await http_client_ptr_->read_and_write(l_req, l_res, boost::asio::use_awaitable);
  if (l_res.result() != boost::beast::http::status::ok)
    throw_exception(doodle_error{"kitsu get task error {} {}", l_res.result(), l_res.body().dump()});

  co_return l_res.body();
}
boost::asio::awaitable<void> kitsu_client::upload_asset_file(
    std::string in_upload_url, FSys::path in_file_path, std::string in_file_field_name
) const {
  boost::scope::scope_exit l_exit{[&]() {
    http_client_ptr_->body_limit_.reset();
    http_client_ptr_->set_timeout(30s);
  }};
  http_client_ptr_->body_limit_ = 100ll * 1024 * 1024 * 1024;  // 100G
  if (auto l_size = FSys::file_size(in_file_path); l_size > 1024 * 1024)
    http_client_ptr_->set_timeout(chrono::seconds(l_size / (50 * 1024)) + 30s);  // 50KB/s 上传速度估算
  boost::beast::http::request<boost::beast::http::file_body> l_req{boost::beast::http::verb::post, in_upload_url, 11};
  set_req_headers(l_req, "application/octet-stream");
  auto l_last_mod_time = chrono::clock_cast<chrono::system_clock>(FSys::last_write_time(in_file_path));
  l_req.set(
      boost::beast::http::field::last_modified,
      fmt::format("{} GMT", fmt::format("{:%a, %d %b %Y %H:%M:%S}", l_last_mod_time))
  );
  l_req.set(boost::beast::http::field::content_disposition, in_file_field_name);
  boost::system::error_code l_ec{};
  l_req.body().open(in_file_path.string().c_str(), boost::beast::file_mode::read, l_ec);
  if (l_ec) throw_exception(doodle_error{"kitsu upload file open file error {} {}", in_file_path, l_ec.message()});

  SPDLOG_LOGGER_INFO(logger_, "开始上传文件 {} 到 {}", in_file_path, in_upload_url);

  boost::beast::http::response<boost::beast::http::string_body> l_res{};
  co_await http_client_ptr_->read_and_write(l_req, l_res, boost::asio::use_awaitable);
  if (l_res.result() != boost::beast::http::status::no_content)
    throw_exception(doodle_error{"kitsu upload file error {} {} {}", in_file_path, l_res.result(), l_res.body()});

  co_return;
}

boost::asio::awaitable<void> kitsu_client::remove_asset_file(std::string in_upload_url) const {
  SPDLOG_INFO("开始备份文件 {}", in_upload_url);
  boost::beast::http::request<boost::beast::http::empty_body> l_req{
      boost::beast::http::verb::delete_, in_upload_url, 11
  };
  set_req_headers(l_req, {});
  boost::beast::http::response<boost::beast::http::string_body> l_res{};
  co_await http_client_ptr_->read_and_write(l_req, l_res, boost::asio::use_awaitable);
  if (l_res.result() != boost::beast::http::status::no_content)
    throw_exception(doodle_error{"kitsu remove file error {} {}", l_res.result(), l_res.body()});
  co_return;
}

boost::asio::awaitable<void> kitsu_client::upload_asset_file_maya(uuid in_task_id, FSys::path in_file_path) const {
  SPDLOG_LOGGER_INFO(logger_, "上传文件 {}", in_file_path);
  return upload_asset_file(
      fmt::format("/api/doodle/data/assets/{}/file/maya", in_task_id), in_file_path,
      base64_encode(in_file_path.filename().generic_string())
  );
}

std::vector<kitsu_client::update_file_arg> kitsu_client::update_file_arg::list_all_project_files(
    const FSys::path& in_project_path, const std::vector<FSys::path>& in_extra_path
) {
  std::vector<update_file_arg> l_out_list{};

  FSys::path l_uproject_dir{};
  FSys::path l_project_path{};
  if (in_project_path.extension() == doodle_config::ue4_uproject_ext) {
    l_uproject_dir = in_project_path.parent_path();
    l_project_path = in_project_path;
  } else {
    l_project_path = ue_exe_ns::find_ue_project_file(in_project_path);
    l_uproject_dir = l_project_path.parent_path();
  }
  DOODLE_CHICK(!l_project_path.empty(), "未能找到工程文件");

  std::vector<FSys::path> l_path_list{};
  if (in_extra_path.empty()) {
    l_path_list.push_back(l_uproject_dir / doodle_config::ue4_content);
    l_path_list.push_back(l_uproject_dir / doodle_config::ue4_config);
    l_path_list.push_back(l_project_path);
  } else {
    l_path_list = in_extra_path;
  }

  for (auto&& l_path : l_path_list) {
    DOODLE_CHICK(FSys::exists(l_path), "文件不存在, 路径 {}", l_path);

    if (FSys::is_directory(l_path)) {
      for (auto&& p : FSys::recursive_directory_iterator(l_path)) {
        if (p.is_directory()) continue;
        update_file_arg l_arg{};
        l_arg.local_path_ = p.path();
        l_arg.field_name_ = p.path().lexically_relative(l_uproject_dir).generic_string();
        l_out_list.push_back(std::move(l_arg));
      }
    } else if (FSys::is_regular_file(l_path)) {
      update_file_arg l_arg{};
      l_arg.local_path_ = l_path;
      l_arg.field_name_ = l_path.lexically_relative(l_uproject_dir).generic_string();
      l_out_list.push_back(std::move(l_arg));
    }
  }
  return l_out_list;
}

boost::asio::awaitable<void> kitsu_client::upload_asset_file_ue(
    uuid in_task_id, std::vector<update_file_arg> in_file_path
) const {
  for (auto&& l_path : in_file_path) {
    SPDLOG_LOGGER_INFO(logger_, "上传文件 {}", l_path.local_path_);
    co_await upload_asset_file(
        fmt::format("/api/doodle/data/assets/{}/file/ue", in_task_id), l_path.local_path_,
        base64_encode(l_path.field_name_)
    );
  }
  co_return;
}

boost::asio::awaitable<void> kitsu_client::upload_asset_file_image(uuid in_task_id, FSys::path in_file_path) const {
  SPDLOG_LOGGER_INFO(logger_, "上传文件 {}", in_file_path);

  return upload_asset_file(
      fmt::format("/api/doodle/data/assets/{}/file/image", in_task_id), in_file_path,
      base64_encode(in_file_path.filename().generic_string())
  );
}

boost::asio::awaitable<void> kitsu_client::upload_shot_animation_maya(uuid in_shot_task_id, FSys::path in_file_path) {
  SPDLOG_LOGGER_INFO(logger_, "上传文件 {}", in_file_path);
  return upload_asset_file(
      fmt::format("/api/doodle/data/shots/{}/file/maya", in_shot_task_id), in_file_path,
      base64_encode(in_file_path.filename().generic_string())
  );
}

boost::asio::awaitable<void> kitsu_client::upload_shot_animation_export_file(
    uuid in_shot_task_id, FSys::path in_dir, FSys::path in_file_name
) {
  SPDLOG_LOGGER_INFO(logger_, "上传文件 {}", in_dir / in_file_name);
  return upload_asset_file(
      fmt::format("/api/doodle/data/shots/{}/file/output", in_shot_task_id), in_dir / in_file_name,
      base64_encode(in_file_name.generic_string())
  );
}
boost::asio::awaitable<void> kitsu_client::upload_shot_animation_other_file(
    uuid in_shot_task_id, FSys::path in_dir, FSys::path in_file_name
) {
  SPDLOG_LOGGER_INFO(logger_, "上传文件 {}", in_dir / in_file_name);
  return upload_asset_file(
      fmt::format("/api/doodle/data/shots/{}/file/other", in_shot_task_id), in_dir / in_file_name,
      base64_encode(in_file_name.generic_string())
  );
}
boost::asio::awaitable<void> kitsu_client::upload_shot_animation_video_file(
    uuid in_shot_task_id, FSys::path in_file_path
) {
  SPDLOG_LOGGER_INFO(logger_, "上传文件 {}", in_file_path);
  return upload_asset_file(
      fmt::format("/api/doodle/data/shots/{}/file/video", in_shot_task_id), in_file_path,
      base64_encode(in_file_path.filename().generic_string())
  );
}
boost::asio::awaitable<void> kitsu_client::upload_shot_animation_ue(
    uuid in_shot_task_id, std::vector<update_file_arg> in_file_path
) const {
  for (auto&& l_path : in_file_path) {
    SPDLOG_LOGGER_INFO(logger_, "上传文件 {}", l_path.local_path_);
    co_await upload_asset_file(
        fmt::format("/api/doodle/data/shots/{}/file/ue", in_shot_task_id), l_path.local_path_,
        base64_encode(l_path.field_name_)
    );
  }
  co_return;
}
boost::asio::awaitable<void> kitsu_client::remove_asset_file_maya(const uuid& in_uuid) {
  return remove_asset_file(fmt::format("/api/doodle/data/assets/{}/file/maya", in_uuid));
}
boost::asio::awaitable<void> kitsu_client::remove_asset_file_ue(const uuid& in_uuid) {
  return remove_asset_file(fmt::format("/api/doodle/data/assets/{}/file/ue", in_uuid));
}
boost::asio::awaitable<void> kitsu_client::remove_asset_file_image(const uuid& in_uuid) {
  return remove_asset_file(fmt::format("/api/doodle/data/assets/{}/file/image", in_uuid));
}
boost::asio::awaitable<void> kitsu_client::remove_shot_animation_maya(const uuid& in_uuid) {
  return remove_asset_file(fmt::format("/api/doodle/data/shots/{}/file/maya", in_uuid));
}
boost::asio::awaitable<void> kitsu_client::remove_shot_animation_export_file(const uuid& in_uuid) {
  return remove_asset_file(fmt::format("/api/doodle/data/shots/{}/file/output", in_uuid));
}

boost::asio::awaitable<nlohmann::json> kitsu_client::get_ue_assembly(uuid in_project_id, uuid in_shot_task_id) const {
  boost::beast::http::request<boost::beast::http::empty_body> l_req{
      boost::beast::http::verb::post,
      fmt::format("/api/actions/projects/{}/shots/{}/run-ue-assembly", in_project_id, in_shot_task_id), 11
  };
  set_req_headers(l_req, {});

  boost::beast::http::response<http::basic_json_body> l_res{};
  co_await http_client_ptr_->read_and_write(l_req, l_res, boost::asio::use_awaitable);
  if (l_res.result() != boost::beast::http::status::created)
    throw_exception(doodle_error{"kitsu get ue assembly error {} {}", l_res.result(), l_res.body().dump()});

  co_return l_res.body();
}

boost::asio::awaitable<void> kitsu_client::comment_task(comment_task_arg in_arg) const {
  if (kitsu_token_.empty()) throw_exception(doodle_error{"kitsu token is empty, can not comment task"});
  uuid l_comment_id = co_await create_comment(
      in_arg.task_id_, in_arg.comment_, in_arg.task_status_id_, in_arg.checklists_, in_arg.links_
  );
  if (in_arg.attach_files_.empty()) co_return;

  uuid l_preview_id = co_await create_preview(in_arg.task_id_, l_comment_id, in_arg.preview_file_source_);
  {  // 上传附件
    boost::beast::http::request<boost::beast::http::file_body> l_req{
        boost::beast::http::verb::post, fmt::format("/api/pictures/preview-files/{}", l_preview_id), 11
    };
    set_req_headers(l_req, std::string{::doodle::http::kitsu::mime_type(in_arg.attach_files_.extension())});
    boost::system::error_code l_ec{};
    l_req.body().open(in_arg.attach_files_.string().c_str(), boost::beast::file_mode::read, l_ec);
    if (l_ec)
      throw_exception(
          doodle_error{"kitsu upload comment preview open file error {} {}", in_arg.attach_files_, l_ec.message()}
      );
    boost::beast::http::response<boost::beast::http::string_body> l_res{};
    co_await http_client_ptr_->read_and_write(l_req, l_res, boost::asio::use_awaitable);
    if (l_res.result() != boost::beast::http::status::ok && l_res.result() != boost::beast::http::status::created)
      throw_exception(
          doodle_error{
              "kitsu upload comment preview error {} {} {}", in_arg.attach_files_, l_res.result(), l_res.body()
          }
      );
  }
  co_return;
}

boost::asio::awaitable<void> kitsu_client::comment_task_compose_video(comment_task_arg in_arg) const {
  if (kitsu_token_.empty()) throw_exception(doodle_error{"kitsu token is empty, can not comment task"});
  uuid l_comment_id = co_await create_comment(
      in_arg.task_id_, in_arg.comment_, in_arg.task_status_id_, in_arg.checklists_, in_arg.links_
  );
  if (in_arg.attach_files_.empty()) co_return;
  uuid l_preview_id = co_await create_preview(in_arg.task_id_, l_comment_id, in_arg.preview_file_source_);
  {  // 上传附件
    boost::beast::http::request<boost::beast::http::file_body> l_req{
        boost::beast::http::verb::post, fmt::format("/api/actions/preview-files/{}/compose-video", l_preview_id), 11
    };
    set_req_headers(l_req, std::string{::doodle::http::kitsu::mime_type(in_arg.attach_files_.extension())});
    boost::system::error_code l_ec{};
    l_req.body().open(in_arg.attach_files_.string().c_str(), boost::beast::file_mode::read, l_ec);
    DOODLE_CHICK(
        !l_ec, "comment_task_compose_video :kitsu upload comment preview open file error {} {}", in_arg.attach_files_,
        l_ec.message()
    );

    boost::beast::http::response<boost::beast::http::string_body> l_res{};
    co_await http_client_ptr_->read_and_write(l_req, l_res, boost::asio::use_awaitable);
    if (l_res.result() != boost::beast::http::status::ok && l_res.result() != boost::beast::http::status::created)
      throw_exception(
          doodle_error{
              "kitsu upload comment preview error {} {} {}", in_arg.attach_files_, l_res.result(), l_res.body()
          }
      );
  }
  co_return;
}

boost::asio::awaitable<nlohmann::json> kitsu_client::get_export_anim_fbx(uuid in_task_id) const {
  boost::beast::http::request<boost::beast::http::empty_body> l_req{
      boost::beast::http::verb::get, fmt::format("/api/actions/tasks/{}/export-anim-fbx", in_task_id), 11
  };
  set_req_headers(l_req, {});

  boost::beast::http::response<http::basic_json_body> l_res{};
  co_await http_client_ptr_->read_and_write(l_req, l_res, boost::asio::use_awaitable);
  if (l_res.result() != boost::beast::http::status::ok && l_res.result() != boost::beast::http::status::created)
    throw_exception(doodle_error{"kitsu get export anim fbx error {} {}", l_res.result(), l_res.body().dump()});

  co_return l_res.body();
}
boost::asio::awaitable<nlohmann::json> kitsu_client::get_task_sync(uuid in_task_id) const {
  boost::beast::http::request<boost::beast::http::empty_body> l_req{
      boost::beast::http::verb::get, fmt::format("/api/actions/tasks/{}/sync", in_task_id), 11
  };
  set_req_headers(l_req);

  boost::beast::http::response<http::basic_json_body> l_res{};
  co_await http_client_ptr_->read_and_write(l_req, l_res, boost::asio::use_awaitable);
  if (l_res.result() != boost::beast::http::status::ok && l_res.result() != boost::beast::http::status::created)
    throw_exception(doodle_error{"kitsu get task sync error {} {}", l_res.result(), l_res.body().dump()});

  co_return l_res.body();
}
boost::asio::awaitable<nlohmann::json> kitsu_client::get_task_assets_update_ue_files(uuid in_task_id) const {
  boost::beast::http::request<boost::beast::http::empty_body> l_req{
      boost::beast::http::verb::get, fmt::format("/api/actions/tasks/{}/assets/update/ue", in_task_id), 11
  };
  set_req_headers(l_req);

  boost::beast::http::response<http::basic_json_body> l_res{};
  co_await http_client_ptr_->read_and_write(l_req, l_res, boost::asio::use_awaitable);
  if (l_res.result() != boost::beast::http::status::ok && l_res.result() != boost::beast::http::status::created)
    throw_exception(
        doodle_error{"kitsu get task assets update ue files error {} {}", l_res.result(), l_res.body().dump()}
    );

  co_return l_res.body();
}

}  // namespace doodle::kitsu