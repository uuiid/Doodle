//
// Created by TD on 2024/2/21.
//

#include <doodle_core/doodle_core_fwd.h>

#include <boost/dynamic_bitset.hpp>
#include <boost/url.hpp>
namespace doodle::http {
class http_function;

using http_function_ptr = std::shared_ptr<http_function>;
class http_route {
 public:
  using action_type = std::function<void(const entt::handle&)>;

 private:
  using map_actin_type = std::vector<http_function_ptr>;
  std::map<boost::beast::http::verb, map_actin_type> actions;

  template <typename type_t, typename = void>
  struct has_verb : std::false_type {};
  template <typename type_t>
  struct has_verb<type_t, std::void_t<decltype(type_t::verb_)>> : std::true_type {};
  template <typename type_t>
  static constexpr bool has_verb_v = has_verb<type_t>::value;

  inline std::vector<std::string> split_str(std::string& in_str) {
    std::vector<std::string> l_vector{};
    boost::split(l_vector, in_str, boost::is_any_of("/"));
    return l_vector;
  }
  // 注册路由
  void reg(boost::beast::http::verb in_verb, std::vector<std::string> in_vector, http_function_ptr in_function);

 public:
  template <typename CompletionHandler>
  http_route& get(std::string url, CompletionHandler&& in_handler) {
    reg(boost::beast::http::verb::get, split_str(url),
        [handler_ = std::forward<CompletionHandler>(in_handler)](const entt::handle& in_handle) {
          not_upgrade_websocket(in_handle, handler_);
        });
    return *this;
  };
  template <typename MsgBody, typename CompletionHandler>
  http_route& put(std::string url, CompletionHandler&& in_handler) {
    reg(boost::beast::http::verb::put, split_str(url),
        [handler_ = std::forward<CompletionHandler>(in_handler)](const entt::handle& in_handle) mutable {
          read_body<MsgBody>(in_handle, std::forward<decltype(handler_)>(handler_));
        });
    return *this;
  };
  template <typename MsgBody, typename CompletionHandler>
  http_route& post(std::string url, CompletionHandler&& in_handler) {
    reg(boost::beast::http::verb::post, split_str(url),
        [handler_ = std::forward<CompletionHandler>(in_handler)](const entt::handle& in_handle) mutable {
          read_body<MsgBody>(in_handle, std::forward<decltype(handler_)>(handler_));
        });
    return *this;
  };

  // 路由分发
  action_type operator()(boost::beast::http::verb in_verb, boost::urls::segments_ref in_segment) const;
};
}  // namespace doodle::http