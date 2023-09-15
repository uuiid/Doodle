//
// Created by td_main on 2023/8/9.
//

#pragma once

#include <boost/beast.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/url.hpp>
namespace doodle::render_farm {
namespace detail {

class http_route {
 public:
  using action_type = std::function<void(const entt::handle&)>;
  class capture_url {
    std::vector<std::string> capture_vector_;
    boost::dynamic_bitset<> capture_bitset_{};

    void set_cap_bit();
    std::tuple<bool, std::map<std::string, std::string>> match_url(boost::urls::segments_ref in_segments_ref) const;

   public:
    using action_type = std::function<void(const entt::handle&, const std::map<std::string, std::string>&)>;

    action_type action_;
    explicit capture_url(std::vector<std::string> in_vector, action_type in_function)
        : capture_vector_{std::move(in_vector)}, action_{std::move(in_function)} {
      set_cap_bit();
    };

    http_route::action_type operator()(boost::urls::segments_ref in_segments_ref) const;
  };

 private:
  using map_actin_type = std::vector<capture_url>;
  std::map<boost::beast::http::verb, map_actin_type> actions;
  map_actin_type websocket_actions;

  template <typename type_t, typename = void>
  struct has_verb : std::false_type {};
  template <typename type_t>
  struct has_verb<type_t, std::void_t<decltype(type_t::verb_)>> : std::true_type {};
  template <typename type_t>
  static constexpr bool has_verb_v = has_verb<type_t>::value;

 public:
  // 注册路由
  void reg(boost::beast::http::verb in_verb, std::vector<std::string> in_vector, capture_url::action_type in_function);
  void reg(std::vector<std::string> in_vector, capture_url::action_type in_function);

  template <typename T, std::enable_if_t<has_verb_v<T>>* = nullptr>
  void reg() {
    T l_reg{};
    reg(l_reg.verb_, l_reg.url_, l_reg);
  }
  template <typename T, std::enable_if_t<!has_verb_v<T>>* = nullptr>
  void reg() {
    T l_reg{};
    reg(l_reg.url_, l_reg);
  }

  // 路由分发
  action_type operator()(boost::beast::http::verb in_verb, boost::urls::segments_ref in_segment) const;
  action_type operator()(boost::urls::segments_ref in_segment) const;
};

}  // namespace detail
}  // namespace doodle::render_farm