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
  inline uuid get_uuid(const std::string& in_str = "id") const {
    if (!capture_map_.contains(in_str))
      throw_exception(http_request_error{boost::beast::http::status::bad_request, "请求参数错误"});
    return from_uuid_str(capture_map_.at(in_str));
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

class url_route_t {
  // SFINAE friendly trait to get a member object pointer's field type
  template <typename T>
  struct object_field_type {};

  template <typename T>
  using object_field_type_t = typename object_field_type<T>::type;

  template <typename F, typename O>
  struct object_field_type<F O::*> : std::enable_if<!std::is_function<F>::value, F> {};
  template <class T>
  struct member_object_type {};

  template <class F, class O>
  struct member_object_type<F O::*> : std::type_identity<O> {};

  template <class T>
  using member_object_type_t = typename member_object_type<T>::type;

 public:
  // uuid regex
  constexpr static auto g_uuid_regex =
      std::string_view{"([0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12})"};
  // year month regex
  constexpr static auto g_year_month_regex     = std::string_view{"([0-9]{4}-[0-9]{2})"};
  // year month day regex
  constexpr static auto g_year_month_day_regex = std::string_view{"([0-9]{4}-[0-9]{2}-[0-9]{2})"};

  struct component_base_t {
    std::regex regex_;
    explicit component_base_t(std::string&& in_str) : regex_(std::move(in_str)) {}
    virtual ~component_base_t() = default;
    virtual bool match(const std::string& in_str) const;
    virtual void set(const std::string& in_str, const std::shared_ptr<void>& in_obj) const;

    uuid convert_uuid(const std::string& in_str) const;
    chrono::year_month convert_year_month(const std::string& in_str) const;
    chrono::year_month_day convert_year_month_day(const std::string& in_str) const;
  };
  // 组件转换
  template <typename T>
  struct component_t : component_base_t {
    // 指向需要转换的类属性指针
    T member_pointer_;
    using object_type = member_object_type_t<T>;
    using field_type  = object_field_type_t<T>;

    template <typename Member_Pointer>
      requires std::is_member_pointer_v<Member_Pointer>
    explicit component_t(std::string&& in_str, Member_Pointer in_target)
        : component_base_t(std::move(in_str)), member_pointer_(in_target) {}
    // 设置属性
    void set(const std::string& in_str, const std::shared_ptr<void>& in_obj) const override {
      *std::static_pointer_cast<object_type>(in_obj).*member_pointer_ = this->convert<field_type>(in_str);
    }

    template <typename T1>
    T1 convert(const std::string& in_str) const;
    template <>
    uuid convert(const std::string& in_str) const {
      return this->convert_uuid(in_str);
    }
    template <>
    chrono::year_month convert(const std::string& in_str) const {
      return this->convert_year_month(in_str);
    }
    template <>
    chrono::year_month_day convert(const std::string& in_str) const {
      return this->convert_year_month_day(in_str);
    }
  };
  std::vector<std::shared_ptr<component_base_t>> component_vector_{};
  url_route_t() = default;

  url_route_t& operator/(std::string&& in_str) {
    component_vector_.push_back(std::make_shared<component_base_t>(std::move(in_str)));
    return *this;
  }
  url_route_t& operator/(const std::shared_ptr<component_base_t>& in_ptr) {
    component_vector_.push_back(in_ptr);
    return *this;
  }
  template <typename Member_Pointer>
  static auto make_component(std::string&& in_str, Member_Pointer in_target) {
    return std::make_shared<component_t<Member_Pointer>>(std::move(in_str), in_target);
  }
  template <typename Member_Pointer>
  static auto make_component(const std::string_view& in_str, Member_Pointer in_target) {
    return make_component<Member_Pointer>(std::string{in_str}, in_target);
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

  explicit http_function(boost::beast::http::verb in_verb, std::string in_url)
      : http_function_base_t(in_verb), capture_vector_(set_cap_bit(in_url)) {}

 public:
  using capture_t = capture_t;

  std::tuple<bool, capture_t> set_match_url(boost::urls::segments_ref in_segments_ref) const override;
};

#define DOODLE_HTTP_FUN_CONST(fun_name, verb_, url, base_fun, ...)                         \
  class BOOST_PP_CAT(BOOST_PP_CAT(fun_name, _), verb_) : public ::doodle::http::base_fun { \
   public:                                                                                 \
    BOOST_PP_CAT(BOOST_PP_CAT(fun_name, _), verb_)(__VA_ARGS__) : base_fun(boost::beast::http::verb::verb_, url)

#define DOODLE_HTTP_FUN(fun_name, verb_, url, base_fun)                                    \
  class BOOST_PP_CAT(BOOST_PP_CAT(fun_name, _), verb_) : public ::doodle::http::base_fun { \
   public:                                                                                 \
    BOOST_PP_CAT(BOOST_PP_CAT(fun_name, _), verb_)() : base_fun(boost::beast::http::verb::verb_, url) {}

#define DOODLE_HTTP_FUN_END() \
  }                           \
  ;
using http_function_ptr = std::shared_ptr<http_function_base_t>;
}  // namespace doodle::http