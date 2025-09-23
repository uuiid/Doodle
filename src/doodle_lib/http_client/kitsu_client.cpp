#include "kitsu_client.h"

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
}  // namespace doodle::kitsu