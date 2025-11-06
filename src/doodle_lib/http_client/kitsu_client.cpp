#include "kitsu_client.h"

#include "doodle_core/configure/config.h"
#include "doodle_core/configure/static_value.h"
#include "doodle_core/core/core_set.h"
#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/exception/exception.h"
#include "doodle_core/metadata/project.h"
#include "doodle_core/metadata/task_type.h"
#include "doodle_core/metadata/working_file.h"

#include <doodle_lib/exe_warp/export_rig_sk.h>
#include <doodle_lib/exe_warp/import_and_render_ue.h>
#include <doodle_lib/exe_warp/ue_exe.h>

#include <boost/asio/awaitable.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/file_body_fwd.hpp>
#include <boost/beast/http/string_body_fwd.hpp>
#include <boost/scope/scope_exit.hpp>

#include <cpp-base64/base64.h>
#include <filesystem>
#include <fmt/compile.h>
#include <nlohmann/json_fwd.hpp>
#include <vector>

namespace doodle::kitsu {

// from json kitsu_client::file_association
void from_json(const nlohmann::json& j, kitsu_client::file_association& fa) {
  j.at("ue_file").get_to(fa.ue_file_);
  j.at("solve_file_").get_to(fa.maya_file_);
  j.at("type").get_to(fa.type_);
}

boost::asio::awaitable<kitsu_client::file_association> kitsu_client::get_file_association(uuid in_task_id) const {
  boost::beast::http::request<boost::beast::http::empty_body> l_req{
      boost::beast::http::verb::get, fmt::format("/api/doodle/file_association/{}", in_task_id), 11
  };
  l_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  l_req.set(boost::beast::http::field::accept, "application/json");
  l_req.set(boost::beast::http::field::host, http_client_ptr_->server_ip_and_port_);
  if (!kitsu_token_.empty())
    l_req.set(boost::beast::http::field::authorization, fmt::format("Bearer {}", kitsu_token_));
  l_req.prepare_payload();
  boost::beast::http::response<http::basic_json_body> l_res{};
  co_await http_client_ptr_->read_and_write(l_req, l_res, boost::asio::use_awaitable);
  if (l_res.result() != boost::beast::http::status::ok)
    throw_exception(doodle_error{"kitsu get file association error"});
  co_return l_res.body().get<file_association>();
}

boost::asio::awaitable<FSys::path> kitsu_client::get_ue_plugin(std::string in_version) const {
  auto l_file_name = fmt::format("Doodle_{}.{}.zip", version::build_info::get().version_str, in_version);
  auto l_temp_path = core_set::get_set().get_cache_root("ue_plugin") / l_file_name;

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
    http_client_ptr_->timeout_ = 30s;
  }};
  http_client_ptr_->body_limit_ = 100ll * 1024 * 1024 * 1024;  // 100G
  http_client_ptr_->timeout_    = 1000s;
  l_req.set(boost::beast::http::field::keep_alive, fmt::format("timeout={}", http_client_ptr_->timeout_.count()));
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
    l_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    l_req.set(boost::beast::http::field::accept, "application/json");
    l_req.set(boost::beast::http::field::host, http_client_ptr_->server_ip_and_port_);
    if (!kitsu_token_.empty())
      l_req.set(boost::beast::http::field::authorization, fmt::format("Bearer {}", kitsu_token_));
    boost::beast::http::response<http::basic_json_body> l_res{};
    co_await http_client_ptr_->read_and_write(l_req, l_res, boost::asio::use_awaitable);
    if (l_res.result() != boost::beast::http::status::ok)
      throw_exception(doodle_error{"kitsu get task error {}", l_res.result()});
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
  l_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  l_req.set(boost::beast::http::field::accept, "application/json");
  l_req.set(boost::beast::http::field::host, http_client_ptr_->server_ip_and_port_);
  if (!kitsu_token_.empty())
    l_req.set(boost::beast::http::field::authorization, fmt::format("Bearer {}", kitsu_token_));
  boost::beast::http::response<http::basic_json_body> l_res{};
  co_await http_client_ptr_->read_and_write(l_req, l_res, boost::asio::use_awaitable);
  if (l_res.result() != boost::beast::http::status::ok)
    throw_exception(doodle_error{"kitsu get task error {}", l_res.result()});

  co_return l_res.body();
}
boost::asio::awaitable<void> kitsu_client::upload_asset_file(
    std::string in_upload_url, FSys::path in_file_path, std::string in_file_field_name
) const {
  boost::beast::http::request<boost::beast::http::file_body> l_req{boost::beast::http::verb::post, in_upload_url, 11};
  l_req.set(boost::beast::http::field::content_description, in_file_field_name);
  l_req.set(boost::beast::http::field::content_type, "application/octet-stream");
  l_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  l_req.set(boost::beast::http::field::accept, "application/json");
  l_req.set(boost::beast::http::field::host, http_client_ptr_->server_ip_and_port_);
  boost::system::error_code l_ec{};
  l_req.body().open(in_file_path.string().c_str(), boost::beast::file_mode::read, l_ec);
  if (l_ec) throw_exception(doodle_error{"kitsu upload file open file error {} {}", in_file_path, l_ec.message()});

  boost::beast::http::response<boost::beast::http::string_body> l_res{};
  co_await http_client_ptr_->read_and_write(l_req, l_res, boost::asio::use_awaitable);
  if (l_res.result() != boost::beast::http::status::ok)
    throw_exception(doodle_error{"kitsu upload file error {} {}", in_file_path, l_res.result()});

  co_return;
}
boost::asio::awaitable<void> kitsu_client::upload_asset_file_maya(uuid in_task_id, FSys::path in_file_path) const {
  boost::scope::scope_exit l_exit{[&]() {
    http_client_ptr_->body_limit_.reset();
    http_client_ptr_->timeout_ = 30s;
  }};
  http_client_ptr_->body_limit_ = 100ll * 1024 * 1024 * 1024;  // 100G
  http_client_ptr_->timeout_    = 1000s;
  return upload_asset_file(
      fmt::format("/api/doodle/data/asset/{}/file/maya", in_task_id), in_file_path,
      base64_encode(in_file_path.filename().generic_string())
  );
}
boost::asio::awaitable<void> kitsu_client::upload_asset_file_ue(
    uuid in_task_id, std::shared_ptr<std::vector<FSys::path>> in_file_path
) const {
  if (!in_file_path) throw_exception(doodle_error{"上传ue资产文件路径不能为空"});
  if (in_file_path->empty()) throw_exception(doodle_error{"上传ue资产文件路径不能为空"});
  boost::scope::scope_exit l_exit{[&]() {
    http_client_ptr_->body_limit_.reset();
    http_client_ptr_->timeout_ = 30s;
  }};
  http_client_ptr_->body_limit_ = 100ll * 1024 * 1024 * 1024;  // 100G
  http_client_ptr_->timeout_    = 1000s;

  auto l_ue_project_file        = ue_exe_ns::find_ue_project_file(in_file_path->front());

  if (l_ue_project_file.extension() != doodle_config::ue4_uproject_ext)
    throw_exception(doodle_error{"上传的文件不是ue工程文件, 或者UE工程内部文件 {}", *in_file_path});

  auto l_uproject_dir = l_ue_project_file.parent_path();

  for (auto&& l_path : *in_file_path) {
    for (auto&& p : FSys::recursive_directory_iterator(l_path)) {
      if (p.is_directory()) continue;
      co_await upload_asset_file(
          fmt::format("/api/doodle/data/asset/{}/file/ue", in_task_id), p.path(),
          base64_encode(p.path().lexically_relative(l_uproject_dir).generic_string())
      );
    }
  }
  co_await upload_asset_file(
      fmt::format("/api/doodle/data/asset/{}/file/ue", in_task_id), l_ue_project_file,
      base64_encode(l_ue_project_file.filename().generic_string())
  );
  co_return;
}

boost::asio::awaitable<void> kitsu_client::upload_asset_file_ue(uuid in_task_id, FSys::path in_file_path) const {
  boost::scope::scope_exit l_exit{[&]() {
    http_client_ptr_->body_limit_.reset();
    http_client_ptr_->timeout_ = 30s;
  }};
  http_client_ptr_->body_limit_ = 100ll * 1024 * 1024 * 1024;  // 100G
  http_client_ptr_->timeout_    = 1000s;
  if (in_file_path.extension() != doodle_config::ue4_uproject_ext)
    throw_exception(doodle_error{"上传的文件不是ue工程文件 {}", in_file_path});

  auto l_uproject_dir = in_file_path.parent_path();
  if (FSys::exists(l_uproject_dir / doodle_config::ue4_content / doodle_config::ue4_prop)) {
    // 如果存在Prop文件夹，则上传Prop文件夹
    for (auto&& p :
         FSys::recursive_directory_iterator(l_uproject_dir / doodle_config::ue4_content / doodle_config::ue4_prop)) {
      if (p.is_directory()) continue;
      co_await upload_asset_file(
          fmt::format("/api/doodle/data/asset/{}/file/ue", in_task_id), p.path(),
          base64_encode(p.path().lexically_relative(l_uproject_dir).generic_string())
      );
    }
  } else {
    // 否则上传工程文件
    for (auto&& p : FSys::recursive_directory_iterator(l_uproject_dir / doodle_config::ue4_content)) {
      if (p.is_directory()) continue;
      co_await upload_asset_file(
          fmt::format("/api/doodle/data/asset/{}/file/ue", in_task_id), p.path(),
          base64_encode(p.path().lexically_relative(l_uproject_dir).generic_string())
      );
    }
    for (auto&& p : FSys::recursive_directory_iterator(l_uproject_dir / doodle_config::ue4_config)) {
      if (p.is_directory()) continue;
      co_await upload_asset_file(
          fmt::format("/api/doodle/data/asset/{}/file/ue", in_task_id), p.path(),
          base64_encode(p.path().lexically_relative(l_uproject_dir).generic_string())
      );
    }
  }

  co_await upload_asset_file(
      fmt::format("/api/doodle/data/asset/{}/file/ue", in_task_id), in_file_path,
      base64_encode(in_file_path.filename().generic_string())
  );
  co_return;
}

boost::asio::awaitable<void> kitsu_client::upload_asset_file_image(uuid in_task_id, FSys::path in_file_path) const {
  boost::scope::scope_exit l_exit{[&]() {
    http_client_ptr_->body_limit_.reset();
    http_client_ptr_->timeout_ = 30s;
  }};
  http_client_ptr_->body_limit_ = 100ll * 1024 * 1024 * 1024;  // 100G
  http_client_ptr_->timeout_    = 1000s;
  return upload_asset_file(
      fmt::format("/api/doodle/data/asset/{}/file/image", in_task_id), in_file_path,
      base64_encode(in_file_path.filename().generic_string())
  );
}
boost::asio::awaitable<nlohmann::json> kitsu_client::get_ue_assembly(uuid in_project_id, uuid in_shot_task_id) const {
  boost::beast::http::request<boost::beast::http::empty_body> l_req{
      boost::beast::http::verb::post,
      fmt::format("/api/actions/projects/{}/shots/{}/run-ue-assembly", in_project_id, in_shot_task_id), 11
  };
  l_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  l_req.set(boost::beast::http::field::accept, "application/json");
  l_req.set(boost::beast::http::field::host, http_client_ptr_->server_ip_and_port_);
  if (!kitsu_token_.empty())
    l_req.set(boost::beast::http::field::authorization, fmt::format("Bearer {}", kitsu_token_));
  boost::beast::http::response<http::basic_json_body> l_res{};
  co_await http_client_ptr_->read_and_write(l_req, l_res, boost::asio::use_awaitable);
  if (l_res.result() != boost::beast::http::status::ok)
    throw_exception(doodle_error{"kitsu get ue assembly error {}", l_res.result()});

  co_return l_res.body();
}
}  // namespace doodle::kitsu