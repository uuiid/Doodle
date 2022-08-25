//
// Created by TD on 2022/4/29.
//

#pragma once

#include <nlohmann/json.hpp>

#include <doodle_core/json_rpc/core/parser_rpc.h>
#include <doodle_core/json_rpc/core/rpc_request.h>

#include <boost/signals2.hpp>
namespace boost::asio {
class io_context;
}

namespace doodle::json_rpc {

class rpc_client {
  class impl;
  std::unique_ptr<impl> ptr;

 public:
  using string_sig = boost::signals2::signal<void(const std::string&)>;
  explicit rpc_client(boost::asio::io_context& in_context, const std::string& in_host, std::uint16_t in_post);
  virtual ~rpc_client();

 protected:
  std::string call_server(const std::string& in_string, bool is_notice);
  void call_server(const std::string& in_string, const string_sig& in_skin);

  template <typename Result_Type, typename Arg, std::enable_if_t<!std::is_same_v<void, Result_Type>, std::int32_t> = 0>
  auto call_fun(const std::string& in_name, Arg args) {
    nlohmann::json l_json{};

    rpc_request l_rpc_request{};
    l_rpc_request.method_   = in_name;
    l_rpc_request.is_notice = false;
    l_rpc_request.params_   = args;

    l_json                  = l_rpc_request;
    std::string l_json_str{};
    l_json_str         = call_server(l_json.dump(), false);

    nlohmann::json l_r = nlohmann::json::parse(l_json_str);
    auto l_rpc_r       = l_r.template get<rpc_reply>();
    if (l_rpc_r.result.index() != rpc_reply::err_index) {
      return std::get<nlohmann::json>(l_rpc_r.result).template get<Result_Type>();
    } else {
      auto l_err_ = std::get<rpc_error>(l_rpc_r.result);
      l_err_.to_throw();
    }
  }

  template <typename Result_Type, std::enable_if_t<!std::is_same_v<void, Result_Type>, std::int32_t> = 0>
  auto call_fun(const std::string& in_name) {
    nlohmann::json l_json{};

    rpc_request l_rpc_request{};
    l_rpc_request.method_   = in_name;
    l_rpc_request.is_notice = false;

    l_json                  = l_rpc_request;
    std::string l_json_str{};
    l_json_str         = call_server(l_json.dump(), false);

    nlohmann::json l_r = nlohmann::json::parse(l_json_str);
    auto l_rpc_r       = l_r.template get<rpc_reply>();
    if (l_rpc_r.result.index() == rpc_reply::err_index) {
      auto l_err_ = std::get<rpc_error>(l_rpc_r.result);
      l_err_.to_throw();
    }
  }

  template <typename Result_Type, bool is_notice_type, typename Arg, std::enable_if_t<std::is_same_v<void, Result_Type>, std::int32_t> = 0>
  auto call_fun(const std::string& in_name, Arg args) {
    nlohmann::json l_json{};

    rpc_request l_rpc_request{};
    l_rpc_request.method_   = in_name;
    l_rpc_request.is_notice = is_notice_type;
    l_rpc_request.params_   = args;

    l_json                  = l_rpc_request;
    std::string l_json_str{};
    if constexpr (is_notice_type) {
      call_server(l_json.dump(), is_notice_type);
      return;
    } else {
      l_json_str = call_server(l_json.dump(), is_notice_type);
    }

    nlohmann::json l_r = nlohmann::json::parse(l_json_str);
    auto l_rpc_r       = l_r.template get<rpc_reply>();
    if (l_rpc_r.result.index() == rpc_reply::err_index) {
      auto l_err_ = std::get<rpc_error>(l_rpc_r.result);
      l_err_.to_throw();
    }
  }

  template <typename Result_Type, bool is_notice_type, std::enable_if_t<std::is_same_v<void, Result_Type>, std::int32_t> = 0>
  auto call_fun(const std::string& in_name) {
    nlohmann::json l_json{};

    rpc_request l_rpc_request{};
    l_rpc_request.method_   = in_name;
    l_rpc_request.is_notice = is_notice_type;

    l_json                  = l_rpc_request;
    std::string l_json_str{};
    if constexpr (is_notice_type) {
      call_server(l_json.dump(), is_notice_type);
      return;
    } else {
      l_json_str = call_server(l_json.dump(), is_notice_type);
    }

    nlohmann::json l_r = nlohmann::json::parse(l_json_str);
    auto l_rpc_r       = l_r.template get<rpc_reply>();
    if (l_rpc_r.result.index() == rpc_reply::err_index) {
      auto l_err_ = std::get<rpc_error>(l_rpc_r.result);
      l_err_.to_throw();
    }
  }

  template <typename Result_Type, typename Arg, std::enable_if_t<!std::is_same_v<void, Result_Type>, std::int32_t> = 0>
  void call_fun(const std::string& in_name, const typename boost::signals2::signal<void(const Result_Type&)>& in_skin, Arg args) {
    nlohmann::json l_json{};

    rpc_request l_rpc_request{};
    l_rpc_request.method_   = in_name;
    l_rpc_request.is_notice = false;
    l_rpc_request.params_   = args;

    l_json                  = l_rpc_request;

    string_sig l_sig{};
    l_sig.connect([&](const std::string& in_string) {
      nlohmann::json l_r = nlohmann::json::parse(in_string);
      auto l_rpc_r       = l_r.template get<rpc_reply>();
      if (l_rpc_r.result.index() != rpc_reply::err_index) {
        in_skin(std::get<nlohmann::json>(l_rpc_r.result).template get<Result_Type>());
      } else {
        auto l_err_ = std::get<rpc_error>(l_rpc_r.result);
        l_err_.to_throw();
      }
    });

    call_server(l_json.dump(), l_sig);
  }

  template <typename Result_Type, std::enable_if_t<!std::is_same_v<void, Result_Type>, std::int32_t> = 0>
  void call_fun(const std::string& in_name, const typename boost::signals2::signal<void(const Result_Type&)>& in_skin) {
    nlohmann::json l_json{};

    rpc_request l_rpc_request{};
    l_rpc_request.method_   = in_name;
    l_rpc_request.is_notice = false;

    l_json                  = l_rpc_request;

    string_sig l_sig{};
    l_sig.connect([&](const std::string& in_string) {
      nlohmann::json l_r = nlohmann::json::parse(in_string);
      auto l_rpc_r       = l_r.template get<rpc_reply>();
      if (l_rpc_r.result.index() != rpc_reply::err_index) {
        in_skin(std::get<nlohmann::json>(l_rpc_r.result).template get<Result_Type>());
      } else {
        auto l_err_ = std::get<rpc_error>(l_rpc_r.result);
        l_err_.to_throw();
      }
    });

    call_server(l_json.dump(), l_sig);
  }
  void close();
};
}  // namespace doodle::json_rpc
