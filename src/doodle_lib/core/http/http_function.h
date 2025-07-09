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

struct capture_id_t {
  uuid id_;
};

class url_route_component_t {
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
  constexpr static auto g_number               = std::string_view{"([0-9]+)"};

  struct component_base_t {
    std::regex regex_;
    explicit component_base_t(std::string&& in_str) : regex_(std::move(in_str)) {}
    virtual ~component_base_t() = default;
    virtual bool match(const std::string& in_str) const;
    virtual bool set(const std::string& in_str, const std::shared_ptr<void>& in_obj) const;

    std::tuple<bool, uuid> convert_uuid(const std::string& in_str) const;
    std::tuple<bool, chrono::year_month> convert_year_month(const std::string& in_str) const;
    std::tuple<bool, chrono::year_month_day> convert_year_month_day(const std::string& in_str) const;
    std::tuple<bool, std::int32_t> convert_number(const std::string& in_str) const;
    virtual const std::type_info& get_type() const { return typeid(void); }
    virtual std::function<std::shared_ptr<void>()> create_object() const { return {}; }
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
    bool set(const std::string& in_str, const std::shared_ptr<void>& in_obj) const override {
      auto [l_result, l_value] = this->convert_uuid(in_str);
      if (l_result) *std::static_pointer_cast<object_type>(in_obj).*member_pointer_ = l_value;
      return l_result;
    }
    std::function<std::shared_ptr<void>()> create_object() const override {
      return [] { return std::make_shared<object_type>(); };
    }
    const std::type_info& get_type() const override { return typeid(object_type); }

    template <typename T1>
    T1 convert(const std::string& in_str) const;
    template <>
    std::tuple<bool, uuid> convert(const std::string& in_str) const {
      return this->convert_uuid(in_str);
    }
    template <>
    std::tuple<bool, chrono::year_month> convert(const std::string& in_str) const {
      return this->convert_year_month(in_str);
    }
    template <>
    std::tuple<bool, chrono::year_month_day> convert(const std::string& in_str) const {
      return this->convert_year_month_day(in_str);
    }
    template <>
    std::tuple<bool, std::int32_t> convert(const std::string& in_str) const {
      return this->convert_number(in_str);
    }
  };

 private:
  std::vector<std::shared_ptr<component_base_t>> component_vector_{};
  std::function<std::shared_ptr<void>()> create_object_{};
  std::type_index object_type_{typeid(void)};

 public:
  std::shared_ptr<void> create_object() const;
  std::vector<std::shared_ptr<component_base_t>>& component_vector() { return component_vector_; }
  const std::vector<std::shared_ptr<component_base_t>>& component_vector() const { return component_vector_; }
  const std::type_index& object_type() const { return object_type_; }

  url_route_component_t() = default;

  url_route_component_t& operator/(std::string&& in_str) {
    component_vector_.push_back(std::make_shared<component_base_t>(std::move(in_str)));
    return *this;
  }
  url_route_component_t& operator/(const std::shared_ptr<component_base_t>& in_ptr);

  template <typename Member_Pointer>
    requires std::is_member_pointer_v<Member_Pointer>
  url_route_component_t& operator/(Member_Pointer in_target) {
    if constexpr (std::is_same_v<std::remove_cv_t<member_object_type_t<Member_Pointer>>, uuid>)
      return operator/(make_cap(g_uuid_regex, in_target));
    else if constexpr (std::is_same_v<std::remove_cv_t<member_object_type_t<Member_Pointer>>, chrono::year_month>)
      return operator/(make_cap(g_year_month_regex, in_target));
    else if constexpr (std::is_same_v<std::remove_cv_t<member_object_type_t<Member_Pointer>>, chrono::year_month_day>)
      return operator/(make_cap(g_year_month_day_regex, in_target));
    else if constexpr (std::is_same_v<std::remove_cv_t<member_object_type_t<Member_Pointer>>, std::int32_t>)
      return operator/(make_cap(g_number, in_target));
    else
      static_assert(false, "not support type");
    return *this;
  }
  template <typename String_T, typename Member_Pointer>
    requires std::is_member_pointer_v<Member_Pointer> && std::is_convertible_v<String_T, std::string>
  url_route_component_t& operator/(const std::tuple<String_T, Member_Pointer>& in_tuple) {
    return operator/(make_cap(std::get<0>(in_tuple), std::get<1>(in_tuple)));
  }

  template <typename Member_Pointer>
    requires std::is_member_pointer_v<Member_Pointer>
  static auto make_cap(std::string&& in_str, Member_Pointer in_target) {
    return std::make_shared<component_t<Member_Pointer>>(std::move(in_str), in_target);
  }
  template <typename Member_Pointer>
    requires std::is_member_pointer_v<Member_Pointer>
  static auto make_cap(const std::string_view& in_str, Member_Pointer in_target) {
    return make_component<Member_Pointer>(std::string{in_str}, in_target);
  }
};

class http_function_base_t {
 protected:
  boost::beast::http::verb verb_;

 public:
  using ucom_t = url_route_component_t;
  // uuid regex
  constexpr static auto g_uuid_regex =
      std::string_view{"([0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12})"};
  // year month regex
  constexpr static auto g_year_month_regex     = std::string_view{"([0-9]{4}-[0-9]{2})"};
  // year month day regex
  constexpr static auto g_year_month_day_regex = std::string_view{"([0-9]{4}-[0-9]{2}-[0-9]{2})"};
  constexpr static auto g_number               = std::string_view{"([0-9]+)"};

  template <typename Member_Pointer>
  static auto make_cap(std::string&& in_str, Member_Pointer in_target) {
    return std::make_shared<url_route_component_t::component_t<Member_Pointer>>(std::move(in_str), in_target);
  }
  template <typename Member_Pointer>
  static auto make_cap(const std::string_view& in_str, Member_Pointer in_target) {
    return make_cap<Member_Pointer>(std::string{in_str}, in_target);
  }

  http_function_base_t() = default;
  explicit http_function_base_t(boost::beast::http::verb in_verb) : verb_{in_verb} {}
  virtual ~http_function_base_t() = default;

  [[nodiscard]] inline boost::beast::http::verb get_verb() const { return verb_; }
  [[nodiscard]] virtual bool has_websocket() const;
  [[nodiscard]] virtual bool is_proxy() const;

  virtual std::tuple<bool, std::shared_ptr<void>> set_match_url(boost::urls::segments_ref in_segments_ref) const = 0;

  virtual boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle)     = 0;
  virtual void websocket_init(session_data_ptr in_handle);
  virtual boost::asio::awaitable<void> websocket_callback(
      boost::beast::websocket::stream<tcp_stream_type> in_stream, session_data_ptr in_handle
  );
};

class http_function : public http_function_base_t {
 protected:
  url_route_component_t url_route_;
  virtual const std::type_info& get_type() const;
  void check_type() const;
  explicit http_function(boost::beast::http::verb in_verb, const url_route_component_t& in_url)
      : http_function_base_t(in_verb), url_route_(in_url) {
    check_type();
  }

 public:
  using capture_t = capture_t;

  std::tuple<bool, std::shared_ptr<void>> set_match_url(boost::urls::segments_ref in_segments_ref) const override;
};
#define DOODLE_HTTP_FUN_TEMPLATE(base_fun)                                                                        \
  template <typename Capture_T = capture_id_t>                                                                    \
  class BOOST_PP_CAT(base_fun, _template) : public base_fun {                                                     \
    boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override { \
      return callback_arg(in_handle, std::static_pointer_cast<Capture_T>(in_handle->capture_));                   \
    }                                                                                                             \
    virtual const std::type_info& get_type() const override { return typeid(Capture_T); }                         \
                                                                                                                  \
   public:                                                                                                        \
    using base_fun::base_fun;                                                                                     \
                                                                                                                  \
    virtual boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(                           \
        session_data_ptr in_handle, const std::shared_ptr<Capture_T>& in_arg                                      \
    ) = 0;                                                                                                        \
  };                                                                                                              \
                                                                                                                  \
  template <>                                                                                                     \
  class BOOST_PP_CAT(base_fun, _template)<void> : public base_fun {                                               \
    boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override { \
      return callback_arg(in_handle);                                                                             \
    }                                                                                                             \
    virtual const std::type_info& get_type() const override { return typeid(void); }                              \
                                                                                                                  \
   public:                                                                                                        \
    using base_fun::base_fun;                                                                                     \
                                                                                                                  \
    virtual boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(                           \
        session_data_ptr in_handle                                                                                \
    ) = 0;                                                                                                        \
  };

DOODLE_HTTP_FUN_TEMPLATE(http_function)

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