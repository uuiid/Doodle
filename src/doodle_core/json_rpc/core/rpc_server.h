//
// Created by TD on 2022/4/29.
//
#pragma once

#include "doodle_core/exception/exception.h"
#include <doodle_core/json_rpc/core/rpc_reply.h>
#include <doodle_core/json_rpc/exception/json_rpc_error.h>

#include <boost/callable_traits.hpp>
#include <boost/callable_traits/args.hpp>
#include <boost/function_types/function_arity.hpp>
#include <boost/function_types/function_type.hpp>
#include <boost/function_types/parameter_types.hpp>
#include <boost/function_types/result_type.hpp>
#include <boost/mpl/erase.hpp>
#include <boost/signals2.hpp>
#include <boost/signals2/connection.hpp>

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>


namespace doodle::json_rpc {
namespace detail {
template <class ParamSequence, std::size_t... Indices>
auto unpack_params(ParamSequence, std::index_sequence<Indices...>)
    -> std::tuple<boost::mpl::at_c<ParamSequence, Indices>...>;
}

class DOODLE_CORE_API rpc_server {
 public:
  using call_fun  = std::function<nlohmann::json(const std::optional<nlohmann::json>&)>;

  using call_     = call_fun;

  using sig_type  = boost::signals2::signal<void(const std::string&)>;
  using slot_type = sig_type::slot_type;

 private:
 protected:
  std::map<std::string, call_> fun_list_{};
  sig_type sig_fun{};

  template <typename... Ts>
  constexpr static auto decay_types(const std::tuple<Ts...>&)
      -> std::tuple<std::remove_cv_t<std::remove_reference_t<Ts>>...>{};

 public:
  rpc_server();
  virtual ~rpc_server();

  void register_fun(const std::string& in_name, const call_fun& in_call);
  boost::signals2::connection register_sig(const slot_type& in_solt);

 public:
  /**
   * 模板注册方法
   * @tparam Fun_T 传入的函数模板
   * @param in_name rpc名称
   * @param in_fun_t 传入的函数
   */
  template <typename Fun_T>
  void register_fun_t(const std::string& in_name, Fun_T&& in_fun_t) {
    using Fun_D_T   = std::decay_t<Fun_T>;
    auto l_call_fun = std::make_shared<Fun_D_T>(std::forward<Fun_D_T>(in_fun_t));
    register_fun(in_name, [l_call_fun](const std::optional<nlohmann::json>& in_arg) -> nlohmann::json {
      /// @brief 分解注册函数中的类型
      using Fun_Result                  = typename boost::callable_traits::return_type_t<Fun_T>;
      //      using Fun_Result = typename decltype(std::function<typename Fun_T>{})::result_type;
      using Fun_Parameter               = typename boost::callable_traits::args_t<Fun_T>;
      constexpr auto Fun_Parameter_Size = std::tuple_size_v<Fun_Parameter>;
      //      constexpr auto Fun_Parameter_Size =
      //      boost::function_types::function_arity<decltype(&Fun_T::operator())>::value - 1;

      //      typedef typename boost::function_types::result_type<Fun_T>::type Fun_Result;

      nlohmann::json json_l{};
      try {
        impl_call_fun<Fun_D_T>(*l_call_fun, in_arg, json_l);
      } catch (const nlohmann::json::parse_error& error) {
        throw_exception(json_rpc::invalid_params_exception{});
      }
      return json_l;
    });
  };

 private:
  template <
      typename Fun, std::enable_if_t<
                        std::tuple_size_v<typename boost::callable_traits::args_t<Fun>> == 0 &&
                        std::is_same_v<typename boost::callable_traits::return_type_t<Fun>, void>>* = nullptr>
  inline static void impl_call_fun(
      Fun& in_fun, const std::optional<nlohmann::json>& in_json, nlohmann::json& out_json
  ) {
    (in_fun)();
  }

  template <
      typename Fun, std::enable_if_t<
                        std::tuple_size_v<typename boost::callable_traits::args_t<Fun>> == 0 &&
                        !std::is_same_v<typename boost::callable_traits::return_type_t<Fun>, void>>* = nullptr>
  inline static void impl_call_fun(
      Fun& in_fun, const std::optional<nlohmann::json>& in_json, nlohmann::json& out_json
  ) {
    out_json = (in_fun)();
  }

  template <
      typename Fun, std::enable_if_t<
                        std::tuple_size_v<typename boost::callable_traits::args_t<Fun>> == 1 &&
                        std::is_same_v<typename boost::callable_traits::return_type_t<Fun>, void>>* = nullptr>
  inline static void impl_call_fun(
      Fun& in_fun, const std::optional<nlohmann::json>& in_json, nlohmann::json& out_json
  ) {
    using Fun_Parameter_Decay = decltype(decay_types(std::declval<boost::callable_traits::args_t<Fun>>()));
    (in_fun)(in_json->template get<std::decay_t<decltype(std::get<0>(std::declval<Fun_Parameter_Decay>()))>>());
  }

  template <
      typename Fun, std::enable_if_t<
                        std::tuple_size_v<typename boost::callable_traits::args_t<Fun>> == 1 &&
                        !std::is_same_v<typename boost::callable_traits::return_type_t<Fun>, void>>* = nullptr>
  inline static void impl_call_fun(
      Fun& in_fun, const std::optional<nlohmann::json>& in_json, nlohmann::json& out_json
  ) {
    using Fun_Parameter_Decay = decltype(decay_types(std::declval<boost::callable_traits::args_t<Fun>>()));
    out_json =
        (in_fun)(in_json->template get<std::decay_t<decltype(std::get<0>(std::declval<Fun_Parameter_Decay>()))>>());
  }

  template <
      typename Fun, std::enable_if_t<
                        (std::tuple_size_v<typename boost::callable_traits::args_t<Fun>> > 1) &&
                        std::is_same_v<typename boost::callable_traits::return_type_t<Fun>, void>>* = nullptr>
  inline static void impl_call_fun(
      Fun& in_fun, const std::optional<nlohmann::json>& in_json, nlohmann::json& out_json
  ) {
    using Fun_Parameter_Decay = decltype(decay_types(std::declval<boost::callable_traits::args_t<Fun>>()));
    std::apply((in_fun), in_json->template get<Fun_Parameter_Decay>());
  }

  template <
      typename Fun, std::enable_if_t<
                        (std::tuple_size_v<typename boost::callable_traits::args_t<Fun>> > 1) &&
                        !std::is_same_v<typename boost::callable_traits::return_type_t<Fun>, void>>* = nullptr>
  inline static void impl_call_fun(
      Fun& in_fun, const std::optional<nlohmann::json>& in_json, nlohmann::json& out_json
  ) {
    using Fun_Parameter_Decay = decltype(decay_types(std::declval<boost::callable_traits::args_t<Fun>>()));
    out_json                  = std::apply((in_fun), in_json->template get<Fun_Parameter_Decay>());
  }

 public:
  virtual std::string operator()(const std::string& in_data) const;
};
class session;

}  // namespace doodle::json_rpc
