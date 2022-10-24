//
// Created by TD on 2022/9/9.
//

#include "dingding_api.h"
#include <doodle_dingding/configure/config.h>
#include <doodle_dingding/metadata/department.h>
#include <doodle_dingding/metadata/request_base.h>
#include <doodle_core/doodle_core.h>
#include <doodle_dingding/fmt_lib/boost_beast_fmt.h>

#include <nlohmann/json.hpp>

namespace doodle {
namespace dingding {
dingding_api::dingding_api(
    const boost::asio::any_io_executor& in_executor,
    boost::asio::ssl::context& in_ssl_context
) : client(in_executor, in_ssl_context) {
  static boost::url l_dinding{dingding_host};
  set_openssl(l_dinding.host());
}
void dingding_api::async_get_token(
    read_access_token_fun&& in
) {
  using req_type = boost::beast::http::request<boost::beast::http::empty_body>;
  using res_type = boost::beast::http::response<boost::beast::http::string_body>;

  /// @brief 初始化l_url{}类
  boost::url l_url{};
  req_type l_req{};///初始化l_req{}类
  boost::url l_method{"gettoken"};

  l_method.params().set("appkey", dingding_config::get().app_key);///设置App的关键字
  l_method.params().set("appsecret", dingding_config::get().app_value);///设置App密码的值

  l_req.method(boost::beast::http::verb::get);///方法用get方法
  boost::urls::resolve(
      boost::urls::url_view{dingding_host},
      l_method,
      l_url
  );
  async_write_read<res_type>(///异步读写
      l_req,
      l_url,
      [l_fu = std::move(in), l_c = shared_from_this()](
          boost::system::error_code in_code,
          const res_type& in_res_type
      ) {
        auto l_j                 = nlohmann::json::parse(in_res_type.body());
        auto l_access_token_body = access_token_body{l_j};///访问令牌
        if (l_access_token_body) {
          throw_exception(l_access_token_body.get_error());
        }
        l_fu(l_access_token_body.result_type());
      }
  );
}
void dingding_api::async_get_departments(///异步找到部门
    const department_ns::department_query& in_query,
    const access_token& in_token,
    dingidng_call_fun&& in_fun
) {
  boost::url l_url{};
  boost::url l_method{"topapi/v2/department/get"};
  l_method.params().set("access_token", in_token.token); ///设置访问令牌
  using req_type = boost::beast::http::request<boost::beast::http::string_body>;
  using res_type = boost::beast::http::response<boost::beast::http::string_body>;
  req_type l_req{};///初始化一个l_req{}类

  l_req.method(boost::beast::http::verb::post);///获取方法用post
  nlohmann::json l_json = in_query;
  l_req.body()          = l_json.dump();///转换成字符串

  DOODLE_LOG_INFO(l_req);

  boost::urls::resolve(
      boost::urls::url_view{dingding_host},
      l_method,
      l_url
  );
  /// @brief 将回调转移到堆中
  /// 将值从 val -> new val
  /// {  std::int32_t l_val{10}; std::int32_t* l_ptr = new std::int32_t{l_val}; }
  /// *l_ptr = 20;  不能 l_val=20;
  /// delete l_ptr
  auto l_call_fun = std::make_shared<dingidng_call_fun>(in_fun);
///@brief 闭包
///***[=](const std::int32_t& in_val)-> bool {} 值 ***///
///***[&](){} 引用***///
  async_write_read<res_type>(///响应类型的异步读写
      l_req,
      l_url,
      [=, l_c = shared_from_this()](
          boost::system::error_code in_code,
          const res_type& in_res_type
      ) {
        DOODLE_LOG_INFO(in_res_type);
        if (in_res_type.body().empty())
          return;
        auto l_j    = nlohmann::json::parse(in_res_type.body());//?
        auto l_body = department_body{l_j};
        if (l_body) {
          throw_exception(l_body.get_error());
        }
        auto l_res = l_body.result_type();
        auto l_msg = std::vector{make_handle()};
        l_msg.front().emplace<department>(l_res);
        boost::asio::post(
            this->get_executor(),
            [l_call_fun, l_msg]() { (*l_call_fun)(l_msg); }
        );
        ;
      }
  );
}
void dingding_api::async_get_departments_user(//找到部门成员
    const user_dd_ns::dep_query& in_query,
    const access_token& in_token,
    dingidng_call_fun&& in_fun
) {
  boost::url l_url{};
  boost::url l_method{"topapi/user/listsimple"};
  l_method.params().set("access_token", in_token.token);
  using req_type = boost::beast::http::request<boost::beast::http::string_body>;
  using res_type = boost::beast::http::response<boost::beast::http::string_body>;
  req_type l_req{};

  l_req.method(boost::beast::http::verb::post);
  nlohmann::json l_json = in_query;///设置nlohmann::json文件中l_json = in_query
  l_req.body()          = l_json.dump();

  DOODLE_LOG_INFO(l_req);

  boost::urls::resolve(
      boost::urls::url_view{dingding_host},
      l_method,
      l_url
  );
  auto l_call_fun = std::make_shared<dingidng_call_fun>(in_fun);

  async_write_read<res_type>(
      l_req,
      l_url,
      [=, l_c = shared_from_this()](
          boost::system::error_code in_code,
          const res_type& in_res_type
      ) {
        DOODLE_LOG_INFO(in_res_type);
        if (in_res_type.body().empty())
          return;
        auto l_j    = nlohmann::json::parse(in_res_type.body());
        auto l_body = user_dd_body{l_j};
        if (l_body) {
          throw_exception(l_body.get_error());
        }
        auto l_res = l_body.result_type().list;
        auto l_msg = l_res |
                     ranges::view::transform([](const user_dd& in) -> entt::handle {
                       auto l_handle = doodle::make_handle();
                       l_handle.emplace<user_dd>(in);
                       return l_handle;
                     }) |
                     ranges::to_vector;

        boost::asio::post(
            this->get_executor(),
            [l_call_fun, l_msg]() { (*l_call_fun)(l_msg); }
        );
        ;
      }
  );
}
void dingding_api::async_find_mobile_user(///找到手机用户
    const user_dd_ns::find_by_mobile& in_query,
    const access_token& in_token,
    dingidng_call_fun&& in_fun
) {
  boost::url l_url{};
  boost::url l_method{"topapi/v2/user/getbymobile"};
  l_method.params().set("access_token", in_token.token);
  using req_type = boost::beast::http::request<boost::beast::http::string_body>;
  using res_type = boost::beast::http::response<boost::beast::http::string_body>;
  req_type l_req{};

  l_req.method(boost::beast::http::verb::post);///post方法
  nlohmann::json l_json = in_query;
  l_req.body()          = l_json.dump();

  DOODLE_LOG_INFO(l_req);

  boost::urls::resolve(
      boost::urls::url_view{dingding_host},
      l_method,
      l_url
  );
  auto l_call_fun = std::make_shared<dingidng_call_fun>(std::move(in_fun));

  async_write_read<res_type>(
      l_req,
      l_url,
      [=, l_c = shared_from_this()](
          boost::system::error_code in_code,
          const res_type& in_res_type
      ) {
        DOODLE_LOG_INFO(in_res_type);
        if (in_res_type.body().empty())
          return;
        auto l_j    = nlohmann::json::parse(in_res_type.body());///l_j=in_res_type.body()解析成json
        auto l_body = user_dd_id_list_body{l_j};
        if (l_body) {
          throw_exception(l_body.get_error());
        }
        auto l_res = l_body.result_type();
        boost::asio::post(
            get_executor(),
            [=]() {
              async_get_user_info(
                  {l_res.userid},
                  in_token,
                  std::move(*l_call_fun)
              );
            }
        );
      }
  );
}
void dingding_api::async_get_user_info(///找到用户信息
    const user_dd_ns::get_user_info& in_query,
    const access_token& in_token,
    dingidng_call_fun&& in_fun
) {
  boost::url l_url{};
  boost::url l_method{"topapi/v2/user/get"};
  l_method.params().set("access_token", in_token.token);
  using req_type = boost::beast::http::request<boost::beast::http::string_body>;
  using res_type = boost::beast::http::response<boost::beast::http::string_body>;
  req_type l_req{};

  l_req.method(boost::beast::http::verb::post);
  nlohmann::json l_json = in_query;
  l_req.body()          = l_json.dump();

  DOODLE_LOG_INFO(l_req);

  boost::urls::resolve(
      boost::urls::url_view{dingding_host},
      l_method,
      l_url
  );
  auto l_call_fun = std::make_shared<dingidng_call_fun>(in_fun);//?

  async_write_read<res_type>(
      l_req,
      l_url,
      [=, l_c = shared_from_this()](
          boost::system::error_code in_code,
          const res_type& in_res_type
      ) {
        DOODLE_LOG_INFO(in_res_type);
        if (in_res_type.body().empty())
          return;
        auto l_j    = nlohmann::json::parse(in_res_type.body());
        auto l_body = user_dd_id_list_body{l_j};
        if (l_body) {
          throw_exception(l_body.get_error());
        }
        auto l_res = l_body.result_type();
        auto l_msg = std::vector{make_handle()};
        l_msg.front().emplace<user_dd>(l_res);

        boost::asio::post(
            this->get_executor(),
            [l_call_fun, l_msg]() { (*l_call_fun)(l_msg); }
        );
        ;
      }
  );
}
void dingding_api::async_get_user_day_attendance(///找到用户的出勤日期
    const attendance::query::get_day_data& in_query,
    const access_token& in_token, dingidng_call_fun&& in_fun
) {
  boost::url l_url{};
  boost::url l_method{"attendance/list"};
  l_method.params().set("access_token", in_token.token);
  using req_type = boost::beast::http::request<boost::beast::http::string_body>;
  using res_type = boost::beast::http::response<boost::beast::http::string_body>;
  req_type l_req{};

  l_req.method(boost::beast::http::verb::post);
  auto l_q              = in_query;
  l_q.limit             = std::max(in_query.limit, 50ll);

  nlohmann::json l_json = l_q;
  l_req.body()          = l_json.dump();

  DOODLE_LOG_INFO(l_req);

  boost::urls::resolve(///将boost::urls::url_view{dingding_host}和l_method合并成一个地址l_url
      boost::urls::url_view{dingding_host},
      l_method,
      l_url
  );
  auto l_call_fun = std::make_shared<dingidng_call_fun>(in_fun);

  async_write_read<res_type>(
      l_req,
      l_url,
      [=, l_c = shared_from_this()](
          boost::system::error_code in_code,
          const res_type& in_res_type
      ) {
        DOODLE_LOG_INFO(in_res_type);
        if (in_res_type.body().empty())
          return;
        auto l_j    = nlohmann::json::parse(in_res_type.body());
        auto l_body = attendance::user_day_updatedata_body{l_j};
        if (l_body) {
          throw_exception(l_body.get_error());
        }
        auto l_res = l_body.result_type();
        auto l_msg = l_res |
                     ranges::view::transform(
                         [](const attendance::day_data& in) -> entt::handle {
                           auto l_h = make_handle();
                           l_h.emplace<attendance::day_data>(in);
                           return l_h;
                         }
                     ) |
                     ranges::to_vector;

        boost::asio::post(
            this->get_executor(),
            [l_call_fun, l_msg]() { (*l_call_fun)(l_msg); }
        );
      }
  );
}
void dingding_api::async_get_user_updatedata_attendance(
    const attendance::query::get_update_data& in_query,
    const access_token& in_token, dingidng_call_fun&& in_fun
) {
  boost::url l_url{};
  boost::url l_method{"topapi/attendance/getupdatedata"};
  l_method.params().set("access_token", in_token.token);
  using req_type = boost::beast::http::request<boost::beast::http::string_body>;
  using res_type = boost::beast::http::response<boost::beast::http::string_body>;
  req_type l_req{};

  l_req.method(boost::beast::http::verb::post);///post方法
  nlohmann::json l_json = in_query;
  l_req.body()          = l_json.dump();

  DOODLE_LOG_INFO(l_req);

  boost::urls::resolve(
      boost::urls::url_view{dingding_host},
      l_method,
      l_url
  );
  auto l_call_fun = std::make_shared<dingidng_call_fun>(in_fun);

  async_write_read<res_type>(
      l_req,
      l_url,
      [=, l_c = shared_from_this()](
          boost::system::error_code in_code,
          const res_type& in_res_type
      ) {
        DOODLE_LOG_INFO(in_res_type);
        if (in_res_type.body().empty())
          return;
        auto l_j    = nlohmann::json::parse(in_res_type.body());
        auto l_body = attendance::user_attendance_body{l_j};
        if (l_body) {
          throw_exception(l_body.get_error());
        }
        auto l_res = l_body.result_type();
        std::vector<entt::handle> l_msg{make_handle()};
        l_msg.front().emplace<attendance::attendance>(std::move(l_res));
        boost::asio::post(
            this->get_executor(),
            [l_call_fun, l_msg]() { (*l_call_fun)(l_msg); }
        );
      }
  );
}

}  // namespace dingding
}  // namespace doodle
