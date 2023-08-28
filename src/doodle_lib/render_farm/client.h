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
  struct computer_list_t {
    using response_type = boost::beast::http::response<render_farm::detail::basic_json_body>;
    using result_type   = std::vector<computer>;
    result_type result_;
    detail::client_core* ptr_;

    boost::beast::http::message_generator operator()();

    result_type operator()(const response_type& in_response);
  };

  // cancel
  inline void cancel() { core_ptr_->cancel(); }

 public:
  template <typename CompletionHandler>
  auto async_computer_list(CompletionHandler&& in_completion) {
    return core_ptr_->async_main(
        boost::asio::make_strand(g_io_context()), std::forward<decltype(in_completion)>(in_completion),
        computer_list_t{}
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

  struct task_list_t {
    using response_type = boost::beast::http::response<render_farm::detail::basic_json_body>;
    using result_type   = std::vector<task_t>;
    detail::client_core* ptr_;

    boost::beast::http::message_generator operator()();

    result_type operator()(const response_type& in_response);
  };

 public:
  template <typename CompletionHandler>
  auto async_task_list(CompletionHandler&& in_completion) {
    return core_ptr_->async_main(
        boost::asio::make_strand(g_io_context()), std::forward<decltype(in_completion)>(in_completion), task_list_t{}
    );
  }
};

}  // namespace doodle
