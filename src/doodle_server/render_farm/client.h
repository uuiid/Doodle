//
// Created by td_main on 2023/8/18.
//
#pragma once
#include <doodle_core/lib_warp/boost_fmt_error.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <doodle_server/render_farm/client_core.h>
#include <doodle_server/render_farm/detail/basic_json_body.h>

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
        [l_fun = std::move(in_completion), logger_ = core_ptr_](boost::beast::error_code ec, const response_type& PH2) {
          log_info(logger_->logger(), fmt::format("{}", PH2.body().dump()));
          try {
            auto l_list = PH2.body().get<std::vector<computer>>();
            l_fun(ec, l_list);
          } catch (const nlohmann::json::exception& e) {
            log_error(logger_->logger(), fmt::format("{}", e.what()));
            BOOST_BEAST_ASSIGN_EC(ec, boost::asio::error::invalid_argument);
            l_fun(ec, std::vector<computer>{});
          }
        }
    );
  }

  struct task_t {
    std::int64_t id_{};
    std::string name_{};
    std::string state_{};
    std::string time_{};
    FSys::path path_{};
    // form json
    friend void from_json(const nlohmann::json& in_json, task_t& out_data) {
      in_json["id"].get_to(out_data.id_);
      in_json["name"].get_to(out_data.name_);
      in_json["status"].get_to(out_data.state_);
      in_json["time"].get_to(out_data.time_);
      in_json["path"].get_to(out_data.path_);
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
        [l_fun = std::move(in_completion), logger_ = core_ptr_](auto&& ec, const response_type& PH2) {
          log_info(logger_->logger(), fmt::format("{}", PH2.body().dump()));
          try {
            auto l_list = PH2.body().get<std::vector<task_t>>();
            l_fun(ec, l_list);
          } catch (nlohmann::json::exception& e) {
            log_error(logger_->logger(), fmt::format("{}", e.what()));
            BOOST_BEAST_ASSIGN_EC(ec, boost::asio::error::invalid_argument);
            l_fun(ec, std::vector<task_t>{});
          }
        }
    );
  }
};

}  // namespace doodle
