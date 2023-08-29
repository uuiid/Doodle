//
// Created by td_main on 2023/8/18.
//
#pragma once
#include <doodle_core/lib_warp/boost_fmt_error.h>

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/render_farm/client_core.h>
#include <doodle_lib/render_farm/detail/basic_json_body.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace doodle {

class client {
  std::shared_ptr<detail::client_core> core_ptr_;

 public:
  explicit client(std::string in_server_ip)
      : core_ptr_(std::make_shared<detail::client_core>(std::move(in_server_ip))) {}
  ~client() = default;

  struct computer {
    std::int64_t id_{};
    std::string name_{};
    std::string state_{};
    // form json
    friend void from_json(const nlohmann::json& in_json, computer& out_data) {
      in_json["id"].get_to(out_data.id_);
      in_json["name"].get_to(out_data.name_);
      in_json["status"].get_to(out_data.state_);
    }
  };
  // cancel
  inline void cancel() { core_ptr_->cancel(); }

 public:
  template <typename CompletionHandler>
  auto async_computer_list(CompletionHandler&& in_completion) {
    boost::beast::http::request<render_farm::detail::basic_json_body> l_request{
        boost::beast::http::verb::get, "/v1/render_farm/computer", 11};
    l_request.keep_alive(true);
    l_request.set(boost::beast::http::field::content_type, "application/json");
    l_request.set(boost::beast::http::field::accept, "application/json");
    using response_type = boost::beast::http::response<render_farm::detail::basic_json_body>;
    return core_ptr_->async_read<response_type>(
        boost::asio::make_strand(g_io_context()), l_request,
        [l_fun = std::move(in_completion)](auto&& PH1, const response_type& PH2) {
          DOODLE_LOG_INFO("{}", PH2.body().dump());
          l_fun(PH1, PH2.body().get<std::vector<computer>>());
        }
    );
  }

  struct task_t {
    std::int64_t id_{};
    std::string name_{};
    std::string state_{};
    // form json
    friend void from_json(const nlohmann::json& in_json, task_t& out_data) {
      in_json["id"].get_to(out_data.id_);
      in_json["name"].get_to(out_data.name_);
      in_json["status"].get_to(out_data.state_);
    }
  };

 public:
  template <typename CompletionHandler>
  auto async_task_list(CompletionHandler&& in_completion) {
    boost::beast::http::request<render_farm::detail::basic_json_body> l_request{
        boost::beast::http::verb::get, "/v1/render_farm/render_job", 11};
    l_request.keep_alive(true);
    l_request.set(boost::beast::http::field::content_type, "application/json");
    l_request.set(boost::beast::http::field::accept, "application/json");

    using response_type = boost::beast::http::response<render_farm::detail::basic_json_body>;
    return core_ptr_->async_read<response_type>(
        boost::asio::make_strand(g_io_context()), l_request,
        [l_fun = std::move(in_completion)](auto&& PH1, const response_type& PH2) {
          DOODLE_LOG_INFO("{}", PH2.body().dump());
          l_fun(PH1, PH2.body().get<std::vector<task_t>>());
        }
    );
  }
};

}  // namespace doodle
