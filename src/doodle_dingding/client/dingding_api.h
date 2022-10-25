//
// Created by TD on 2022/9/9.
//
#pragma once

#include <doodle_core/doodle_core.h>
#include <doodle_core/lib_warp/entt_warp.h>
#include <doodle_core/metadata/time_point_wrap.h>

#include <doodle_dingding/client/client.h>
#include <doodle_dingding/doodle_dingding_fwd.h>

#include <boost/asio/async_result.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

#include <nlohmann/json_fwd.hpp>

namespace doodle {
class time_point_wrap;
}

namespace doodle::dingding {

class access_token;
using dingidng_call_fun     = std::function<void(const std::vector<entt::handle>&)>;
using read_access_token_fun = std::function<void(const access_token&)>;

class DOODLE_DINGDING_API dingding_api : public client {
 private:
  void async_get_user_info(
      const user_dd_ns::get_user_info& in_query,
      const std::shared_ptr<std::function<void(const boost::system::error_code&, const dingding::user_dd&)>>& in_call
  );

  class impl;
  std::unique_ptr<impl> ptr;

  void tocken_delay() const;

  void async_run(const std::shared_ptr<std::function<void()>>& in_call);

  template <
      typename Req_t, typename Res_t, typename Return_t, typename Arg_t, typename Call_t, char* in_url,
      std::int32_t in_method>
  void async_template(const Arg_t& in_arg, const std::shared_ptr<Call_t>& in_call);

  void async_get_departments_impl(
      const department_ns::department_query& in_query,
      const std::shared_ptr<std::function<void(const boost::system::error_code&, const department&)>>& in_call
  );

  void async_find_mobile_user_impl(
      const dingding::user_dd_ns::find_by_mobile& in_get_user_info,
      const std::shared_ptr<std::function<void(const boost::system::error_code&, const dingding::user_dd&)>>& in_call
  );

  void async_get_user_updatedata_attendance_impl(
      const dingding::attendance::query::get_update_data& in_get_user_info,
      const std::shared_ptr<
          std::function<void(const boost::system::error_code&, const dingding::attendance::attendance&)>>& in_call
  );
  void async_get_user_updatedata_attendance_list_impl(
      const doodle::time_point_wrap& in_time_begin, const doodle::time_point_wrap& in_time_end,
      const std::string& in_user_id,
      const std::shared_ptr<
          std::function<void(const boost::system::error_code&, const std::vector<dingding::attendance::attendance>&)>>&
          in_call,
      const std::shared_ptr<std::vector<dingding::attendance::attendance>>& in_list = {}
  );

 public:
  constexpr static const std::string_view dingding_host{"https://oapi.dingtalk.com"};

  explicit dingding_api(const boost::asio::any_io_executor& in_executor, boost::asio::ssl::context& in_ssl_context);
  virtual ~dingding_api();
  /**
   * @brief 获取钉钉的授权令牌
   * @param in  回调
   */
  void async_get_token(read_access_token_fun&& in);

  /**
   * 获取部门
   * @param in_query
   * @param in_token
   * @param in_fun
   */
  template <typename CompletionHandler>
  auto async_get_departments(std::int32_t in_dep_id, CompletionHandler&& in_handler) {
    using call_type = void(const boost::system::error_code& in_code, const department& in_department);
    return boost::asio::async_initiate<CompletionHandler, call_type>(
        [in_dep_id, this](auto&& in_completion_handler) {
          auto l_fun = std::make_shared<std::function<call_type>>(
              std::forward<decltype(in_completion_handler)>(in_completion_handler)
          );
          auto l_call = std::make_shared<std::function<void()>>([l_fun, in_dep_id, this]() {
            async_get_departments_impl({in_dep_id}, l_fun);
          });
          async_run(l_call);
        },
        in_handler
    );
  }

  /**
   * @brief 根据id获取部门用户
   * https://oapi.dingtalk.com/topapi/v2/department/get
   * https://open.dingtalk.com/document/orgapp-server/query-department-details0-v2
   * @param in_query 基本上传入id即可
   * @param in_token 令牌
   * @param in_fun 回调
   */
  //  void async_get_departments_user(
  //      const user_dd_ns::dep_query& in_query, const access_token& in_token, dingidng_call_fun&& in_fun
  //  );

  /**
   * @brief 根据手机获取用户
   * https://open.dingtalk.com/document/orgapp-server/query-users-by-phone-number
   * @param in_query 手机号
   * @param in_fun 回调
   */
  template <typename CompletionHandler>
  auto async_find_mobile_user(const std::string& in_mobile, CompletionHandler&& in_handler) {
    using call_type = void(const boost::system::error_code& in_code, const dingding::user_dd& in_department);
    return boost::asio::async_initiate<CompletionHandler, call_type>(
        [&in_mobile, this](auto&& in_completion_handler) {
          auto l_fun = std::make_shared<std::function<call_type>>(
              std::forward<decltype(in_completion_handler)>(in_completion_handler)
          );
          auto l_call = std::make_shared<std::function<void()>>([l_fun, in_mobile, this]() {
            async_find_mobile_user_impl({in_mobile}, l_fun);
          });
          async_run(l_call);
        },
        in_handler
    );
  }

  /**
   * @brief 获取考勤
   * https://open.dingtalk.com/document/orgapp-server/obtain-the-attendance-update-data
   * @param in_query
   * @param in_token
   * @param in_fun
   */
  template <typename CompletionHandler>
  auto async_get_user_updatedata_attendance(
      const doodle::time_point_wrap& in_time, const std::string& in_user_id, CompletionHandler&& in_handler
  ) {
    using call_type = void(const boost::system::error_code&, const dingding::attendance::attendance&);
    return boost::asio::async_initiate<CompletionHandler, call_type>(
        [&in_time, in_user_id, this](auto&& in_completion_handler) {
          auto l_fun = std::make_shared<std::function<call_type>>(
              std::forward<decltype(in_completion_handler)>(in_completion_handler)
          );
          auto l_call = std::make_shared<std::function<void()>>([l_fun, in_time = in_time, in_user_id, this]() {
            async_get_user_updatedata_attendance_impl({in_time, in_user_id}, l_fun);
          });
          async_run(l_call);
        },
        in_handler
    );
  }

  /**
   * @brief 获取考勤列表的一个方便函数
   *
   * @param in_time_begin 开始时间
   * @param in_time_end 结束时间
   * @param in_user_id 用户id
   * @param in_handler 处理句柄
   */
  template <typename CompletionHandler>
  auto async_get_user_updatedata_attendance_list(
      const doodle::time_point_wrap& in_time_begin, const doodle::time_point_wrap& in_time_end,
      const std::string& in_user_id, CompletionHandler&& in_handler
  ) {
    using call_type = void(const boost::system::error_code&, const std::vector<dingding::attendance::attendance>&);
    return boost::asio::async_initiate<CompletionHandler, call_type>(
        [&in_time_begin, &in_time_end, in_user_id, this](auto&& in_completion_handler) {
          auto l_fun = std::make_shared<std::function<call_type>>(
              std::forward<decltype(in_completion_handler)>(in_completion_handler)
          );
          auto l_call =
              std::make_shared<std::function<void()>>([l_fun, in_time_begin, in_time_end, in_user_id, this]() {
                async_get_user_updatedata_attendance_list_impl(in_time_begin, in_time_end, in_user_id, l_fun);
              });
          async_run(l_call);
        },
        in_handler
    );
  }
};

}  // namespace doodle::dingding
