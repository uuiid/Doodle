//
// Created by td_main on 2023/8/9.
//

#include "url_route_get.h"

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/exception/exception.h>
#include <doodle_core/thread_pool/process_message.h>

#include "doodle_server_fwd.h"
#include <doodle_server/render_farm/detail/computer.h>
#include <doodle_server/render_farm/detail/ue4_task.h>
#include <magic_enum.hpp>
namespace doodle::render_farm::detail {

namespace {
#if NDEBUG
auto repository_path{R"(//192.168.10.218/Doodletemp)"};
#else
auto repository_path{R"(//192.168.20.59/UE_Config/Doodletemp)"};
#endif
}  // namespace

void get_root_type::operator()(
    const entt::handle& in_handle, const boost::beast::http::request<boost::beast::http::empty_body>& in_req
) const {
  boost::beast::http::response<boost::beast::http::string_body> l_response{
      boost::beast::http::status::ok, in_req.version()
  };
  l_response.body() = "hello world";
  l_response.keep_alive(in_handle.get<session::request_parser_empty_body>()->keep_alive());
  l_response.insert(boost::beast::http::field::content_type, "text/html");
  l_response.prepare_payload();
  session::do_write{in_handle, std::move(l_response)}.run();
}

void get_log_type_get::operator()(
    const entt::handle& in_handle, const boost::beast::http::request<boost::beast::http::empty_body>& in_req
) const {
  auto l_logger = in_handle.get<socket_logger>().logger_;
  auto l_cap    = in_handle.get<session::capture_url>().get<entt::id_type>("handle");

  boost::system::error_code l_ec{};
  if (!l_cap) {
    BOOST_BEAST_ASSIGN_EC(l_ec, error_enum::invalid_handle);
    log_warn(l_logger, fmt::format("未找到句柄id :{}", l_ec.message()));
    session::do_write::send_error_code(in_handle, l_ec, boost::beast::http::status::bad_request);
  }
  auto l_h = entt::handle{*g_reg(), num_to_enum<entt::entity>(*l_cap)};
  if (l_h && l_h.all_of<process_message>()) {
    boost::beast::http::response<boost::beast::http::string_body> l_response{
        boost::beast::http::status::ok, in_req.version()
    };
    l_response.body() = l_h.get<process_message>().info_log();
    l_response.keep_alive(in_req.keep_alive());
    l_response.insert(boost::beast::http::field::content_type, "text/html");
    l_response.prepare_payload();
    session::do_write{in_handle, std::move(l_response)}.run();
  } else {
    BOOST_BEAST_ASSIGN_EC(l_ec, error_enum::invalid_handle);
    log_error(l_logger, fmt::format("未找到句柄id :{}", l_h));
    session::do_write::send_error_code(in_handle, l_ec, boost::beast::http::status::not_found);
  }
}
void get_err_type_get::operator()(
    const entt::handle& in_handle, const boost::beast::http::request<boost::beast::http::empty_body>& in_req
) const {
  auto l_logger = in_handle.get<socket_logger>().logger_;
  auto l_cap    = in_handle.get<session::capture_url>().get<entt::id_type>("handle");

  boost::system::error_code l_ec{};
  if (!l_cap) {
    BOOST_BEAST_ASSIGN_EC(l_ec, error_enum::invalid_handle);
    log_warn(l_logger, fmt::format("未找到句柄id :{}", l_ec.message()));
    session::do_write::send_error_code(in_handle, l_ec, boost::beast::http::status::bad_request);
    return;
  }
  auto l_h = entt::handle{*g_reg(), num_to_enum<entt::entity>(*l_cap)};

  if (l_h && l_h.all_of<process_message>()) {
    boost::beast::http::response<boost::beast::http::string_body> l_response{
        boost::beast::http::status::ok, in_req.version()
    };
    l_response.body() = l_h.get<process_message>().err_log();
    l_response.keep_alive(in_req.keep_alive());
    l_response.insert(boost::beast::http::field::content_type, "text/html");
    l_response.prepare_payload();
    session::do_write{in_handle, std::move(l_response)}.run();
  } else {
    BOOST_BEAST_ASSIGN_EC(l_ec, error_enum::invalid_handle);
    log_error(l_logger, fmt::format("未找到句柄id :{}", l_h));
    session::do_write::send_error_code(in_handle, l_ec, boost::beast::http::status::not_found);
  }
}
namespace {
struct computer_tmp {
  std::string name;
  std::string status;
  entt::entity id;
  explicit computer_tmp(const computer& in_computer, entt::entity in_id)
      : name(in_computer.name()), status(magic_enum::enum_name(in_computer.status())), id(in_id) {}

  friend void to_json(nlohmann::json& j, const computer_tmp& in_tmp) {
    j["name"]   = in_tmp.name;
    j["status"] = in_tmp.status;
    j["id"]     = in_tmp.id;
  }
};
struct render_job_tmp {
  entt::entity id_{};
  std::string name_{};
  std::string status_{};
  std::string time_{};

  FSys::path repository_path_{};
  explicit render_job_tmp(const ue4_task& in_task, entt::entity in_id)
      : name_(in_task.arg().ProjectPath),
        status_(fmt::format(
            "{} {}", in_task.is_assign() ? "已分配"s : "未分配"s,
            magic_enum::enum_name(entt::handle{*g_reg(), in_id}.get<process_message>().get_state())
        )),
        id_(in_id),
        time_(fmt::format("{:%H:%M:%S}", entt::handle{*g_reg(), in_id}.get<process_message>().get_time())),
        repository_path_{
            FSys::path{repository_path} / FSys::path{in_task.arg().ProjectPath}.stem() / in_task.arg().out_file_path
        } {}
  friend void to_json(nlohmann::json& j, const render_job_tmp& in_tmp) {
    j["id"]     = in_tmp.id_;
    j["name"]   = in_tmp.name_;
    j["status"] = in_tmp.status_;
    j["time"]   = in_tmp.time_;
    j["path"]   = in_tmp.repository_path_;
  }
};

}  // namespace

void render_job_type_get::operator()(
    const entt::handle& in_handle, const boost::beast::http::request<boost::beast::http::empty_body>& in_req
) const {
  auto l_view = g_reg()->view<ue4_task>().each();
  auto l_ids  = l_view | ranges::views::transform([](auto&& in_item) -> render_job_tmp {
                 auto& l_task = std::get<1>(in_item);
                 return render_job_tmp{l_task, std::get<0>(in_item)};
               }) |
               ranges::to_vector;

  boost::beast::http::response<basic_json_body> l_response{boost::beast::http::status::ok, in_req.version()};
  l_response.body() = l_ids;
  l_response.keep_alive(in_req.keep_alive());
  l_response.insert(boost::beast::http::field::content_type, "application/json");
  l_response.prepare_payload();
  session::do_write{in_handle, std::move(l_response)}.run();
}

void computer_reg_type_get::operator()(
    const entt::handle& in_handle, const boost::beast::http::request<boost::beast::http::empty_body>& in_req
) const {
  auto l_view = g_reg()->view<render_farm::computer>().each();
  auto l_ids  = l_view | ranges::views::transform([](auto&& in_item) -> computer_tmp {
                 auto& l_computer = std::get<1>(in_item);
                 return computer_tmp{l_computer, std::get<0>(in_item)};
               }) |
               ranges::to_vector;
  boost::beast::http::response<basic_json_body> l_response{boost::beast::http::status::ok, in_req.version()};
  l_response.body() = l_ids;
  l_response.keep_alive(in_req.keep_alive());
  l_response.insert(boost::beast::http::field::content_type, "application/json");
  l_response.prepare_payload();
  session::do_write{in_handle, std::move(l_response)}.run();
}
void repository_type_get::operator()(
    const entt::handle& in_handle, const boost::beast::http::request<boost::beast::http::empty_body>& in_req
) const {
  nlohmann::json l_json{};
  l_json["path"] = repository_path;
  boost::beast::http::response<basic_json_body> l_response{boost::beast::http::status::ok, in_req.version()};

  l_response.body() = l_json;
  l_response.keep_alive(in_req.keep_alive());
  l_response.insert(boost::beast::http::field::content_type, "application/json");
  l_response.prepare_payload();
  session::do_write{in_handle, std::move(l_response)}.run();
}
}  // namespace doodle::render_farm::detail