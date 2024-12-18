//
// Created by TD on 2024/2/21.
//
#pragma once
#include "doodle_core/doodle_core_fwd.h"
#include <doodle_core/core/wait_op.h>
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

  inline std::string get(const std::string& in_str) const { return capture_map_.at(in_str); }

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
  bool is_proxy_{false};

 public:
  http_function_base_t() = default;
  explicit http_function_base_t(
      boost::beast::http::verb in_verb,
      std::function<boost::asio::awaitable<boost::beast::http::message_generator>(session_data_ptr)> in_callback,
      std::function<void(const websocket_route_ptr&, const session_data_ptr&)> in_callback_websocket,
      bool in_is_proxy = false
  )
      : verb_{in_verb},
        callback_{std::move(in_callback)},
        websocket_callback_{std::move(in_callback_websocket)},
        is_proxy_(in_is_proxy) {}
  virtual ~http_function_base_t() = default;

  [[nodiscard]] inline boost::beast::http::verb get_verb() const { return verb_; }
  [[nodiscard]] inline bool has_websocket() const { return static_cast<bool>(websocket_callback_); }
  [[nodiscard]] inline bool is_proxy() const { return is_proxy_; }

  virtual std::tuple<bool, capture_t> set_match_url(boost::urls::segments_ref in_segments_ref) const = 0;

  std::function<boost::asio::awaitable<boost::beast::http::message_generator>(session_data_ptr)> callback_;
  std::function<void(const websocket_route_ptr&, const session_data_ptr&)> websocket_callback_;
};

class http_function : public http_function_base_t {
  struct capture_data_t {
    std::string name;
    bool is_capture;
  };

  static std::vector<capture_data_t> set_cap_bit(std::string& in_str);

  const std::vector<capture_data_t> capture_vector_;

 public:
  using capture_t = capture_t;

  explicit http_function(
      boost::beast::http::verb in_verb, std::string in_url,
      std::function<boost::asio::awaitable<boost::beast::http::message_generator>(session_data_ptr)> in_callback
  )
      : http_function_base_t(in_verb, std::move(in_callback), {}, false), capture_vector_(set_cap_bit(in_url)) {}

  explicit http_function(
      boost::beast::http::verb in_verb, std::string in_url,
      std::function<boost::asio::awaitable<boost::beast::http::message_generator>(session_data_ptr)> in_callback,
      std::function<void(const websocket_route_ptr&, const session_data_ptr&)> in_callback_websocket
  )
      : http_function_base_t(in_verb, std::move(in_callback), std::move(in_callback_websocket), false),
        capture_vector_(set_cap_bit(in_url)) {}

  std::tuple<bool, capture_t> set_match_url(boost::urls::segments_ref in_segments_ref) const override;
};

using http_function_ptr = std::shared_ptr<http_function_base_t>;
}  // namespace doodle::http