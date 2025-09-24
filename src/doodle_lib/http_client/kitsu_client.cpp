#include "kitsu_client.h"

#include "doodle_core/configure/config.h"
#include "doodle_core/core/core_set.h"
#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/exception/exception.h"
#include "doodle_core/metadata/task_type.h"
#include "doodle_core/metadata/working_file.h"

#include <boost/asio/awaitable.hpp>

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

  boost::beast::http::request<boost::beast::http::empty_body> l_req{
      boost::beast::http::verb::get, fmt::format("/api/actions/tasks/{}/working-file", in_task_id), 11
  };
  l_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  l_req.set(boost::beast::http::field::accept, "application/json");
  boost::beast::http::response<http::basic_json_body> l_res{};
  co_await http_client_ptr_->read_and_write(l_req, l_res, boost::asio::use_awaitable);
  if (l_res.result() != boost::beast::http::status::ok) throw_exception(doodle_error{"kitsu get task maya file error {}", l_res.result()});
  co_return l_res.body().get<working_file>().path_;
}

}  // namespace doodle::kitsu