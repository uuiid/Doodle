//
// Created by TD on 2024/2/21.
//
#pragma once
#include "doodle_core/doodle_core_fwd.h"
#include <doodle_core/core/wait_op.h>

#include "boost/algorithm/string.hpp"
#include "boost/dynamic_bitset.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/url.hpp>
namespace doodle::http {
class http_function {
  struct capture_data_t {
    std::string name;
    bool is_capture;
  };

  static std::vector<capture_data_t> set_cap_bit(std::string& in_str);

  const boost::beast::http::verb verb_;
  const std::vector<capture_data_t> capture_vector_;

 public:
  struct capture_t {
    std::map<std::string, std::string> capture_map_;
    capture_t() = default;
    explicit capture_t(std::map<std::string, std::string> in_map) : capture_map_(std::move(in_map)) {}

    template <typename T, std::enable_if_t<std::is_arithmetic_v<T>>* = nullptr>
    std::optional<T> get(const std::string& in_str) const {
      if (capture_map_.find(in_str) != capture_map_.end()) {
        return boost::lexical_cast<T>(capture_map_.at(in_str));
      }
      return {};
    }
    template <typename T, std::enable_if_t<!std::is_arithmetic_v<T>>* = nullptr>
    std::optional<T> get(const std::string& in_str) const {
      if (capture_map_.find(in_str) != capture_map_.end()) {
        return capture_map_.at(in_str);
      }
      return {};
    }
  };

  explicit http_function(
      boost::beast::http::verb in_verb, std::string in_url, std::function<void(entt::handle)> in_callback
  )
      : verb_{in_verb}, capture_vector_(set_cap_bit(in_url)), callback_(std::move(in_callback)) {}

  [[nodiscard]] inline boost::beast::http::verb get_verb() const { return verb_; }
  std::tuple<bool, capture_t> set_match_url(boost::urls::segments_ref in_segments_ref) const;

  std::function<void(entt::handle)> callback_;
};
using http_function_ptr = std::shared_ptr<http_function>;

}  // namespace doodle::http
