//
// Created by TD on 2024/2/21.
//

#include "doodle_core/doodle_core_fwd.h"
#include <doodle_core/core/wait_op.h>

#include "boost/algorithm/string.hpp"
#include "boost/dynamic_bitset.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/url.hpp>
namespace doodle::http {
class http_function {
  static inline std::vector<std::string> split_str(std::string& in_str) {
    std::vector<std::string> l_vector{};
    boost::split(l_vector, in_str, boost::is_any_of("/"));
    return l_vector;
  }
  static inline boost::dynamic_bitset<> set_cap_bit(const std::vector<std::string>& in_vector) {
    boost::dynamic_bitset<> l_bitset(in_vector.size());
    for (size_t i = 0; i < in_vector.size(); ++i) {
      if (in_vector[i].front() == '{' && in_vector[i].back() == '}') {
        l_bitset.set(i);
      }
    }
    return l_bitset;
  }

  struct capture_data_t {
    std::string name;
    bool is_capture;
  };

  static inline std::vector<capture_data_t> set_cap_bit(std::string& in_str) {
    std::vector<std::string> l_vector{};
    boost::split(l_vector, in_str, boost::is_any_of("/"));
    std::vector<capture_data_t> l_capture_vector{l_vector.size()};
    for (size_t i = 0; i < l_vector.size(); ++i) {
      if (l_vector[i].front() == '{' && l_vector[i].back() == '}') {
        l_capture_vector[i].name       = l_vector[i].substr(1, l_vector[i].size() - 2);
        l_capture_vector[i].is_capture = true;
      }
    }
    return l_capture_vector;
  }

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

  explicit http_function(boost::beast::http::verb in_verb, std::string in_url)
      : verb_{in_verb}, capture_vector_(set_cap_bit(in_url)) {}
  virtual ~http_function()                                = default;

  // copy constructor
  http_function(const http_function& in_other)            = default;
  // move constructor
  http_function(http_function&& in_other)                 = default;
  // copy assignment
  http_function& operator=(const http_function& in_other) = default;
  // move assignment
  http_function& operator=(http_function&& in_other)      = default;

  [[nodiscard]] inline boost::beast::http::verb get_verb() const { return verb_; }
  std::tuple<bool, capture_t> set_match_url(boost::urls::segments_ref in_segments_ref) const;

  virtual void operator()(const entt::handle& in_handle) const = 0;
};
template <typename MsgBody>
class http_method_base : public http_function {
 public:
};

class http_method_web_socket : public http_function {
 protected:
  virtual void operator_call(const entt::handle& in_handle) const = 0;
  void upgrade_websocket(const entt::handle& in_handle) const;

 public:
  explicit http_method_web_socket(std::string in_url)
      : http_function(boost::beast::http::verb::get, std::move(in_url)) {}
  void operator()(const entt::handle& in_handle) const override;
};

}  // namespace doodle::http
