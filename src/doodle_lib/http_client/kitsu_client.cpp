#include "kitsu_client.h"

#include "doodle_core/configure/config.h"
#include "doodle_core/core/core_set.h"
#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/exception/exception.h"
#include "doodle_core/metadata/project.h"
#include "doodle_core/metadata/task_type.h"
#include "doodle_core/metadata/working_file.h"
#include <doodle_lib/exe_warp/export_rig_sk.h>

#include <boost/asio/awaitable.hpp>

#include <vector>

namespace doodle::kitsu {

// from json kitsu_client::file_association
void from_json(const nlohmann::json& j, kitsu_client::file_association& fa) {
  j.at("ue_file").get_to(fa.ue_file_);
  j.at("solve_file_").get_to(fa.maya_file_);
  j.at("type").get_to(fa.type_);
}

boost::asio::awaitable<kitsu_client::file_association> kitsu_client::get_file_association(
    const uuid& in_task_id
) const {
  boost::beast::http::request<boost::beast::http::empty_body> l_req{
      boost::beast::http::verb::get, fmt::format("/api/doodle/file_association/{}", in_task_id), 11
  };
  l_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  l_req.set(boost::beast::http::field::accept, "application/json");
  l_req.prepare_payload();
  boost::beast::http::response<http::basic_json_body> l_res{};
  co_await http_client_ptr_->read_and_write(l_req, l_res, boost::asio::use_awaitable);
  if (l_res.result() != boost::beast::http::status::ok)
    throw_exception(doodle_error{"kitsu get file association error"});
  co_return l_res.body().get<file_association>();
}

boost::asio::awaitable<FSys::path> kitsu_client::get_ue_plugin(const std::string& in_version) const {
  auto l_file_name = fmt::format("Doodle_{}.{}.zip", version::build_info::get().version_str, in_version);
  auto l_temp_path = core_set::get_set().get_cache_root("ue_plugin") / l_file_name;
  if (FSys::exists(l_temp_path)) co_return l_temp_path;

  boost::beast::http::request<boost::beast::http::empty_body> l_req{
      boost::beast::http::verb::get, fmt::format("/Plugins/{}", l_file_name), 11
  };
  l_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  l_req.set(boost::beast::http::field::accept, "*/*");
  boost::beast::http::response<boost::beast::http::file_body> l_res{};
  boost::system::error_code l_ec{};
  l_res.body().open(l_temp_path.string().c_str(), boost::beast::file_mode::write, l_ec);
  if (l_ec) throw_exception(doodle_error{"kitsu get ue plugin open file error"});
  if (!l_res.body().is_open()) throw_exception(doodle_error{"kitsu get ue plugin open file error"});
  http_client_ptr_->body_limit_ = 2ll * 1024 * 1024 * 1024;  // 2G
  co_await http_client_ptr_->read_and_write(l_req, l_res, boost::asio::use_awaitable);
  http_client_ptr_->body_limit_.reset();
  if (l_res.result() != boost::beast::http::status::ok) throw_exception(doodle_error{"kitsu get ue plugin error"});
  co_return l_temp_path;
}

boost::asio::awaitable<FSys::path> kitsu_client::get_task_maya_file(const uuid& in_task_id) const {
  static std::set<uuid> g_has_maya_file_task{
      task_type::get_character_id(), task_type::get_ground_model_id(), task_type::get_binding_id()
  };
  if (g_has_maya_file_task.contains(in_task_id)) throw_exception(doodle_error{"任务类型不支持获取maya文件"});
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
    boost::beast::http::response<http::basic_json_body> l_res{};
    co_await http_client_ptr_->read_and_write(l_req, l_res, boost::asio::use_awaitable);
    if (l_res.result() != boost::beast::http::status::ok)
      throw_exception(doodle_error{"kitsu get task error {}", l_res.result()});
    l_json_task_full = l_res.body().get<nlohmann::json>();
    l_prj            = l_json_task_full.at("project").get<project>();
    l_list           = l_json_task_full.at("working_files").get<std::vector<working_file>>();
  }

  auto l_it = ranges::find_if(l_list, [&](const working_file& i) { return i.software_type_ == software_enum::maya; });
  if (l_it == l_list.end()) throw_exception(doodle_error{"没有找到对应的maya working file"});
  auto l_path = l_prj.path_ / l_it->path_;
  if (l_it->path_.empty() || !FSys::exists(l_path)) throw_exception(doodle_error{"maya working file 文件不存在"});

  co_return l_path;
}
boost::asio::awaitable<std::shared_ptr<async_task>> kitsu_client::get_generate_uesk_file_arg(
    const uuid& in_task_id
) const {
  if (in_task_id != task_type::get_binding_id()) throw_exception(doodle_error{"任务类型不支持生成 ue sk 文件"});

  nlohmann::json l_json_task_full{};
  {
    boost::beast::http::request<boost::beast::http::empty_body> l_req{
        boost::beast::http::verb::get, fmt::format("/api/data/tasks/{}/full", in_task_id), 11
    };
    l_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    l_req.set(boost::beast::http::field::accept, "application/json");
    l_req.set(boost::beast::http::field::host, http_client_ptr_->server_ip_and_port_);
    boost::beast::http::response<http::basic_json_body> l_res{};
    co_await http_client_ptr_->read_and_write(l_req, l_res, boost::asio::use_awaitable);
    if (l_res.result() != boost::beast::http::status::ok)
      throw_exception(doodle_error{"kitsu get task error {}", l_res.result()});
    l_json_task_full = l_res.body().get<nlohmann::json>();
  }
  // todo: 这个还缺少第二次查询实体数据, 获取UE路径
  co_return std::make_shared<export_rig_sk_arg>();
}

}  // namespace doodle::kitsu