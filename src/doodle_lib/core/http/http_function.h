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

// 初始化

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
  constexpr static auto g_file_name            = std::string_view{R"((^.+\.[A-Za-z]{3}$))"};

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
    std::tuple<bool, FSys::path> convert_file_name(const std::string& in_str) const;

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
      auto [l_result, l_value] = this->convert<field_type>(in_str);
      if (l_result) *std::static_pointer_cast<object_type>(in_obj).*member_pointer_ = l_value;
      return l_result;
    }
    std::function<std::shared_ptr<void>()> create_object() const override {
      return [] { return std::make_shared<object_type>(); };
    }
    const std::type_info& get_type() const override { return typeid(object_type); }

    template <typename T1>
    std::tuple<bool, T1> convert(const std::string& in_str) const;
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
    template <>
    std::tuple<bool, FSys::path> convert(const std::string& in_str) const {
      return this->convert_file_name(in_str);
    }
  };

 private:
  std::vector<std::shared_ptr<component_base_t>> component_vector_{};
  std::function<std::shared_ptr<void>()> create_object_{};
  std::type_index object_type_{typeid(void)};

  explicit url_route_component_t(
      const std::vector<std::shared_ptr<component_base_t>>& in_component_vector,
      const std::function<std::shared_ptr<void>()>& in_create_object, const std::type_index& in_object_type
  )
      : component_vector_(in_component_vector), create_object_(in_create_object), object_type_(in_object_type) {}
  class initializer_t;
  friend url_route_component_t::initializer_t operator""_url(char const* in_str, std::size_t);
  class initializer_t {
   public:
    struct com {
      std::string str_;
      bool is_capture_;
      std::shared_ptr<component_base_t> obj_;

      bool operator==(const bool& in_) const { return is_capture_; }
      bool operator!=(const bool& in_) const { return !is_capture_; }
    };

   private:
    std::string url_path_;
    std::vector<com> component_vector_;
    std::size_t pos_{};
    std::size_t capture_count_{};

    void next_capture();

    void parse_url_path();
    template <typename Member_Pointer>
      requires std::is_member_pointer_v<Member_Pointer>
    initializer_t& add_cap(const std::string_view& in_str, Member_Pointer in_target) {
      BOOST_ASSERT(component_vector_.at(pos_).is_capture_);
      component_vector_.at(pos_).obj_ = std::make_shared<component_t<Member_Pointer>>(
          fmt::vformat(component_vector_.at(pos_).str_, fmt::make_format_args(in_str)), in_target
      );
      next_capture();
      return *this;
    }
    template <typename Member_Pointer>
      requires std::is_member_pointer_v<Member_Pointer>
    initializer_t& add_cap_(Member_Pointer in_target) {
      using field_type = std::remove_cv_t<object_field_type_t<Member_Pointer>>;
      static_assert(
          std::is_same_v<field_type, uuid> || std::is_same_v<field_type, chrono::year_month> ||
              std::is_same_v<field_type, chrono::year_month_day> || std::is_same_v<field_type, std::int32_t> ||
              std::is_same_v<field_type, FSys::path>,
          "not support type"
      );

      if constexpr (std::is_same_v<field_type, uuid>)
        return add_cap(g_uuid_regex, in_target);
      else if constexpr (std::is_same_v<field_type, chrono::year_month>)
        return add_cap(g_year_month_regex, in_target);
      else if constexpr (std::is_same_v<field_type, chrono::year_month_day>)
        return add_cap(g_year_month_day_regex, in_target);
      else if constexpr (std::is_same_v<field_type, std::int32_t>)
        return add_cap(g_number, in_target);
      else if constexpr (std::is_same_v<field_type, FSys::path>)
        return add_cap(g_file_name, in_target);
      return *this;
    }
    template <typename... Args>
    initializer_t& add_cap_args(Args... args) {
      BOOST_ASSERT(capture_count_ == sizeof...(args));
      (add_cap_(args), ...);
      return *this;
    }

   public:
    constexpr explicit initializer_t(const char* in_str, std::size_t in_len) : url_path_{in_str, in_len} {}
    template <typename... Args>
    initializer_t& operator()(Args... args) {
      parse_url_path();
      (add_cap_(args), ...);
      return *this;
    }
    std::vector<std::shared_ptr<component_base_t>> get_component_vector() const;
    operator url_route_component_t() const;
  };

 public:
  // 初始化列表
  std::vector<std::shared_ptr<component_base_t>>& component_vector() { return component_vector_; }
  const std::vector<std::shared_ptr<component_base_t>>& component_vector() const { return component_vector_; }

  std::tuple<bool, std::shared_ptr<http_function>> set_match_url(
      boost::urls::segments_ref in_segments_ref, const std::shared_ptr<http_function>& in_data
  ) const;

  url_route_component_t() = default;
};
url_route_component_t::initializer_t operator""_url(char const* in_str, std::size_t);

class http_function {
 public:
  virtual ~http_function() = default;
  [[nodiscard]] virtual bool has_websocket() const;
  boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle);

  virtual boost::asio::awaitable<boost::beast::http::message_generator> other_callback(session_data_ptr in_handle);
  virtual boost::asio::awaitable<boost::beast::http::message_generator> get(session_data_ptr in_handle);
  virtual boost::asio::awaitable<boost::beast::http::message_generator> put(session_data_ptr in_handle);
  virtual boost::asio::awaitable<boost::beast::http::message_generator> post(session_data_ptr in_handle);
  virtual boost::asio::awaitable<boost::beast::http::message_generator> options(session_data_ptr in_handle);
  virtual boost::asio::awaitable<boost::beast::http::message_generator> delete_(session_data_ptr in_handle);
  virtual void websocket_init(session_data_ptr in_handle);
  virtual void websocket_callback(
      boost::beast::websocket::stream<tcp_stream_type> in_stream, session_data_ptr in_handle
  );

  [[nodiscard]] virtual std::shared_ptr<http_function> clone() const = 0;
};
template <typename Self>
class http_function_template : public http_function {
 public:
  std::shared_ptr<http_function> clone() const override { return std::make_shared<Self>(*this); }
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
using http_function_ptr = std::shared_ptr<http_function>;
}  // namespace doodle::http