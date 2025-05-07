//
// Created by TD on 2024/2/21.
//
#pragma once
#include "doodle_core/doodle_core_fwd.h"
#include <doodle_core/logger/logger.h>

#include <doodle_lib/core/http/http_session_data.h>

#include "boost/algorithm/string.hpp"
#include "boost/dynamic_bitset.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/url.hpp>

#include "http_route.h"

namespace doodle::http {
struct capture_t {
  std::map<std::string, std::string> capture_map_;
  capture_t() = default;

  explicit capture_t(std::map<std::string, std::string> in_map) : capture_map_(std::move(in_map)) {}

  inline std::string get(const std::string& in_str) const {
    if (!capture_map_.contains(in_str))
      throw_exception(http_request_error{boost::beast::http::status::bad_request, "请求参数错误"});
    return capture_map_.at(in_str);
  }

  template <typename T>
    requires std::is_arithmetic_v<T>
  std::optional<T> get(const std::string& in_str) const {
    if (capture_map_.find(in_str) != capture_map_.end()) {
      try {
        return boost::lexical_cast<T>(capture_map_.at(in_str));
      } catch (const boost::bad_lexical_cast& in_err) {
        default_logger_raw()->log(log_loc(), level::err, "get arithmetic error: {}", in_err.what());
        return {};
      }
    }
    return {};
  }
};

class http_function_base_t {
 protected:
  boost::beast::http::verb verb_;

 public:
  http_function_base_t() = default;
  explicit http_function_base_t(boost::beast::http::verb in_verb) : verb_{in_verb} {}
  virtual ~http_function_base_t() = default;

  [[nodiscard]] inline boost::beast::http::verb get_verb() const { return verb_; }
  [[nodiscard]] virtual bool has_websocket() const;
  [[nodiscard]] virtual bool is_proxy() const;

  virtual std::tuple<bool, capture_t> set_match_url(boost::urls::segments_ref in_segments_ref) const         = 0;

  virtual boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) = 0;
  virtual void websocket_init(session_data_ptr in_handle);
  virtual boost::asio::awaitable<void> websocket_callback(
      boost::beast::websocket::stream<tcp_stream_type> in_stream, session_data_ptr in_handle
  );
};

class http_function : public http_function_base_t {
 protected:
  struct capture_data_t {
    std::string name;
    bool is_capture;
  };

  static std::vector<capture_data_t> set_cap_bit(std::string& in_str);

  const std::vector<capture_data_t> capture_vector_;
  std::function<boost::asio::awaitable<boost::beast::http::message_generator>(session_data_ptr)> callback_;

  explicit http_function(boost::beast::http::verb in_verb, std::string in_url)
      : http_function_base_t(in_verb), capture_vector_(set_cap_bit(in_url)) {}

 public:
  using capture_t = capture_t;

  explicit http_function(
      boost::beast::http::verb in_verb, std::string in_url,
      std::function<boost::asio::awaitable<boost::beast::http::message_generator>(session_data_ptr)> in_callback
  )
      : http_function_base_t(in_verb), capture_vector_(set_cap_bit(in_url)), callback_(std::move(in_callback)) {}

  std::tuple<bool, capture_t> set_match_url(boost::urls::segments_ref in_segments_ref) const override;
  boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
};
#define DOODLE_HTTP_FUN(fun_name, verb_, url, base_fun)                                    \
  class BOOST_PP_CAT(BOOST_PP_CAT(fun_name, _), verb_) : public ::doodle::http::base_fun { \
   public:                                                                                 \
    BOOST_PP_CAT(BOOST_PP_CAT(fun_name, _), verb_)() : base_fun(boost::beast::http::verb::verb_, url) {}

#define DOODLE_HTTP_FUN_END() \
  }                           \
  ;
using http_function_ptr = std::shared_ptr<http_function_base_t>;
}  // namespace doodle::http