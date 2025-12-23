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
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/url.hpp>

#include "http_route.h"
#include <magic_enum/magic_enum_all.hpp>
#include <string>

namespace doodle::http {

struct capture_id_t {
  uuid id_;
};

// 初始化
class url_route_component_base_t {
 public:
  virtual ~url_route_component_base_t() = default;
  virtual std::tuple<bool, std::shared_ptr<http_function>> set_match_url(
      boost::urls::segments_ref in_segments_ref, const std::shared_ptr<http_function>& in_data
  ) const = 0;
};
using url_route_component_ptr = std::shared_ptr<url_route_component_base_t>;
class url_route_component_t : public url_route_component_base_t {
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
  constexpr static auto g_file_name            = std::string_view{R"((.+\.[A-Za-z0-9]{3}$))"};
  constexpr static auto g_enum                 = std::string_view{"([A-Za-z0-9_]+)"};

  struct component_base_t {
    virtual ~component_base_t()                                                            = default;

    virtual void set(const std::string& in_str, const std::shared_ptr<void>& in_obj) const = 0;

    uuid convert_uuid(const std::string& in_str) const;
    chrono::year_month convert_year_month(const std::string& in_str) const;
    chrono::year_month_day convert_year_month_day(const std::string& in_str) const;
    std::int32_t convert_number(const std::string& in_str) const;
    FSys::path convert_file_name(const std::string& in_str) const;
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
    explicit component_t(Member_Pointer in_target) : member_pointer_(in_target) {}
    // 设置属性
    void set(const std::string& in_str, const std::shared_ptr<void>& in_obj) const override {
      auto l_value                                                    = this->convert<field_type>(in_str);
      *std::static_pointer_cast<object_type>(in_obj).*member_pointer_ = l_value;
    }

    template <typename T1>
    T1 convert(const std::string& in_str) const;

    template <typename Enum_T>
      requires magic_enum::is_scoped_enum_v<Enum_T>
    Enum_T convert(const std::string& in_str) const {
      return magic_enum::enum_cast<Enum_T>(in_str).value();
    }

    template <typename Uuid_T>
      requires std::is_same_v<Uuid_T, uuid>
    Uuid_T convert(const std::string& in_str) const {
      return this->convert_uuid(in_str);
    }

    template <typename Year_Month_T>
      requires std::is_same_v<Year_Month_T, chrono::year_month>
    Year_Month_T convert(const std::string& in_str) const {
      return this->convert_year_month(in_str);
    }
    template <typename Year_Month_Day_T>
      requires std::is_same_v<Year_Month_Day_T, chrono::year_month_day>
    Year_Month_Day_T convert(const std::string& in_str) const {
      return this->convert_year_month_day(in_str);
    }
    template <typename Number_T>
      requires std::is_same_v<Number_T, std::int32_t>
    Number_T convert(const std::string& in_str) const {
      return this->convert_number(in_str);
    }

    template <typename Path_T>
      requires std::is_same_v<Path_T, FSys::path>
    Path_T convert(const std::string& in_str) const {
      return this->convert_file_name(in_str);
    }
  };

 private:
  using component_ptr      = std::shared_ptr<component_base_t>;
  using component_vector_t = std::vector<component_ptr>;
  component_vector_t component_vector_{};
  std::regex url_regex_{};

  class initializer_t;
  friend url_route_component_t::initializer_t operator""_url(char const* in_str, std::size_t);
  class initializer_t {
    friend url_route_component_t::initializer_t operator""_url(char const* in_str, std::size_t);

    std::string url_path_;
    component_vector_t component_;
    std::size_t capture_count_{};

    void parse_url_path();

    template <typename Member_Pointer>
      requires std::is_member_pointer_v<Member_Pointer>
    auto create_cap_regex(Member_Pointer in_target) {
      using field_type = std::remove_cv_t<object_field_type_t<Member_Pointer>>;
      static_assert(
          std::is_same_v<field_type, uuid> || std::is_same_v<field_type, chrono::year_month> ||
              std::is_same_v<field_type, chrono::year_month_day> || std::is_same_v<field_type, std::int32_t> ||
              std::is_same_v<field_type, FSys::path> || magic_enum::is_scoped_enum_v<field_type>,
          "not support type"
      );

      if constexpr (std::is_same_v<field_type, uuid>)
        return g_uuid_regex;
      else if constexpr (std::is_same_v<field_type, chrono::year_month>)
        return g_year_month_regex;
      else if constexpr (std::is_same_v<field_type, chrono::year_month_day>)
        return g_year_month_day_regex;
      else if constexpr (std::is_same_v<field_type, std::int32_t>)
        return g_number;
      else if constexpr (std::is_same_v<field_type, FSys::path>)
        return g_file_name;
      else if constexpr (magic_enum::is_scoped_enum_v<field_type>) {
        constexpr auto color_names = magic_enum::enum_names<field_type>();
        return fmt::format("({})", fmt::join(color_names, "|"));
      }
    }
    template <typename Member_Pointer>
      requires std::is_member_pointer_v<Member_Pointer>
    auto create_cap_ptr(Member_Pointer in_target) {
      using field_type = std::remove_cv_t<object_field_type_t<Member_Pointer>>;
      static_assert(
          std::is_same_v<field_type, uuid> || std::is_same_v<field_type, chrono::year_month> ||
              std::is_same_v<field_type, chrono::year_month_day> || std::is_same_v<field_type, std::int32_t> ||
              std::is_same_v<field_type, FSys::path> || magic_enum::is_scoped_enum_v<field_type>,
          "not support type"
      );
      return std::make_shared<component_t<Member_Pointer>>(in_target);
    }

    template <typename... Args>
    auto fmt_url_path(Args&&... args) {
      BOOST_ASSERT(capture_count_ == sizeof...(args));
      url_path_ = fmt::vformat(url_path_, fmt::make_format_args(args...));
      return *this;
    }

   public:
    constexpr explicit initializer_t(const char* in_str, std::size_t in_len) : url_path_{in_str, in_len} {}
    template <typename... Args>
    initializer_t& operator()(Args... args) {
      fmt_url_path(create_cap_regex(args)...);
      component_ = {create_cap_ptr(args)...};
      return *this;
    }
    component_vector_t get_component_vector() const;
    operator url_route_component_ptr() const;
  };

 public:
  explicit url_route_component_t(const std::string& in_url_regex, const component_vector_t& in_component_vector)
      : component_vector_(in_component_vector), url_regex_(in_url_regex) {}
  // 初始化列表
  component_vector_t& component_vector() { return component_vector_; }
  const component_vector_t& component_vector() const { return component_vector_; }

  std::tuple<bool, std::shared_ptr<http_function>> set_match_url(
      boost::urls::segments_ref in_segments_ref, const std::shared_ptr<http_function>& in_data
  ) const override;
};
url_route_component_t::initializer_t operator""_url(char const* in_str, std::size_t);

class http_function {
 protected:
  virtual void parse_header(const session_data_ptr& in_handle);

 public:
  virtual ~http_function() = default;
  [[nodiscard]] virtual bool has_websocket() const;
  boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle);

  virtual boost::asio::awaitable<boost::beast::http::message_generator> other_callback(session_data_ptr in_handle);
  virtual boost::asio::awaitable<boost::beast::http::message_generator> get(session_data_ptr in_handle);
  virtual boost::asio::awaitable<boost::beast::http::message_generator> put(session_data_ptr in_handle);
  virtual boost::asio::awaitable<boost::beast::http::message_generator> post(session_data_ptr in_handle);
  virtual boost::asio::awaitable<boost::beast::http::message_generator> options(session_data_ptr in_handle);
  virtual boost::asio::awaitable<boost::beast::http::message_generator> head(session_data_ptr in_handle);
  virtual boost::asio::awaitable<boost::beast::http::message_generator> patch(session_data_ptr in_handle);
  virtual boost::asio::awaitable<boost::beast::http::message_generator> delete_(session_data_ptr in_handle);
  virtual void websocket_init(session_data_ptr in_handle);
  virtual void websocket_callback(
      boost::beast::websocket::stream<tcp_stream_type> in_stream, session_data_ptr in_handle
  );

  [[nodiscard]] virtual std::shared_ptr<http_function> clone() const = 0;
};
template <typename Self, typename Base = http_function>
class http_function_template : public Base {
 public:
  std::shared_ptr<http_function> clone() const override { return std::make_shared<Self>(*this); }
  template <typename... Args>
  explicit http_function_template(Args&&... args) : Base(std::forward<Args>(args)...) {}
};

#define DOODLE_HTTP_FUN_OVERRIDE(method) \
  virtual boost::asio::awaitable<boost::beast::http::message_generator> method(session_data_ptr in_handle) override;

#define DOODLE_HTTP_FUN_C(fun_name, base_fun)                                                           \
  class fun_name : public base_fun {                                                                    \
    using base_type = base_fun;                                                                         \
    std::shared_ptr<http_function> clone() const override { return std::make_shared<fun_name>(*this); } \
                                                                                                        \
   public:
// fun_name() = default;

#define DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(class_name, method) \
  boost::asio::awaitable<boost::beast::http::message_generator> class_name::method(session_data_ptr in_handle)

#define DOODLE_HTTP_FUN(fun_name) DOODLE_HTTP_FUN_C(fun_name, ::doodle::http::http_function)
#define DOODLE_HTTP_FUN_END() \
  }                           \
  ;
using http_function_ptr = std::shared_ptr<http_function>;
}  // namespace doodle::http