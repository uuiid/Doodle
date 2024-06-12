#include "kitsu_supplement.h"

#include <doodle_app/app/app_command.h>

#include <doodle_lib/core/http/http_listener.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/http_method/computing_time.h>
#include <doodle_lib/http_method/dingding_attendance.h>
#include <doodle_lib/http_method/sqlite/kitsu_backend_sqlite.h>
#include <doodle_lib/http_method/user_http.h>
namespace doodle::launch {
struct kitsu_supplement_args_t {
  std::string kitsu_ip_;
  std::string kitsu_port_;
  std::uint16_t port_;
  FSys::path db_path_{};

  std::string kitsu_token_{};
  // 公司
  struct dingding_company_t {
    boost::uuids::uuid id_;
    std::string app_key_;
    std::string app_secret_;

    std::string name_;
    // form json
    friend void from_json(const nlohmann::json& in_json, dingding_company_t& out_obj) {
      std::string l_id_str = in_json.at("id").get<std::string>();
      out_obj.id_          = boost::uuids::string_generator{}(l_id_str);
      in_json.at("app_key").get_to(out_obj.app_key_);
      in_json.at("app_secret").get_to(out_obj.app_secret_);
      in_json.at("name").get_to(out_obj.name_);
    }
  };
  std::vector<dingding_company_t> dingding_company_list_{};

  // form json
  friend void from_json(const nlohmann::json& in_json, kitsu_supplement_args_t& out_obj) {
    in_json.at("kitsu_ip").get_to(out_obj.kitsu_ip_);
    in_json.at("kitsu_port").get_to(out_obj.kitsu_port_);
    in_json.at("kitsu_token").get_to(out_obj.kitsu_token_);
    in_json.at("port").get_to(out_obj.port_);
    in_json.at("db_path").get_to(out_obj.db_path_);
    in_json.at("dingding_company_list").get_to(out_obj.dingding_company_list_);
  }
};
bool kitsu_supplement_t::operator()(const argh::parser& in_arh, std::vector<std::shared_ptr<void>>& in_vector) {
  auto& l_save = g_ctx().emplace<http::kitsu_backend_sqlite>();

  kitsu_supplement_args_t l_args{.kitsu_ip_ = "192.168.40.182", .kitsu_port_ = "80", .port_ = 50025};

  if (auto l_file_path = in_arh({"config"}); l_file_path) {
    auto l_json = nlohmann::json::parse(FSys::ifstream{FSys::from_quotation_marks(l_file_path.str())});
    try {
      l_args = l_json.get<kitsu_supplement_args_t>();
    } catch (const std::exception& e) {
      default_logger_raw()->log(log_loc(), level::err, e.what());
      return true;
    } catch (...) {
      default_logger_raw()->log(log_loc(), level::err, boost::current_exception_diagnostic_information());

      return;
    }
  }

  // 开始监听取消信号
  using signal_t = boost::asio::signal_set;
  auto signal_   = std::make_shared<signal_t>(g_io_context(), SIGINT, SIGTERM);
  signal_->async_wait([](boost::system::error_code in_error_code, int in_sig) {
    if (in_error_code) {
      if (!app_base::GetPtr()->is_stop())
        default_logger_raw()->log(log_loc(), level::err, "信号错误: {}", in_error_code.message());
      return;
    }
    default_logger_raw()->log(log_loc(), level::warn, "收到信号 {} {}", in_error_code.message(), in_sig);
    app_base::GetPtr()->stop_app(1);
  });
  in_vector.emplace_back(signal_);

  // 初始化 ssl
  auto l_ssl_ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12_client);
  in_vector.emplace_back(l_ssl_ctx);

  // 初始化数据库
  {
    g_pool_db().set_path(l_args.db_path_);
    auto l_db_conn = g_pool_db().get_connection();
    l_save.init(l_db_conn);
  }
  {
    // 初始化 kitsu 客户端
    auto l_client = g_ctx().emplace<std::shared_ptr<kitsu::kitsu_client>>(
        std::make_shared<kitsu::kitsu_client>(l_args.kitsu_ip_, l_args.kitsu_port_)
    );
    l_client->set_access_token(std::string{l_args.kitsu_token_});

    // 初始化钉钉客户端
    auto& l_d = g_ctx().emplace<dingding::dingding_company>();

    for (auto&& l_c : l_args.dingding_company_list_) {
      l_d.company_info_map_
          .emplace(
              l_c.id_,
              dingding::dingding_company::company_info{
                  .corp_id     = l_c.id_,
                  .app_key     = l_c.app_key_,
                  .app_secret  = l_c.app_secret_,
                  .name        = l_c.name_,
                  .client_ptr_ = std::make_shared<dingding::client>(*l_ssl_ctx)
              }
          )
          .first->second.client_ptr_->access_token(l_c.app_key_, l_c.app_secret_, true);
    }
  }
  // 初始化路由
  auto l_rout_ptr = std::make_shared<http::http_route>();
  http::reg_computing_time(*l_rout_ptr);
  http::reg_dingding_attendance(*l_rout_ptr);
  http::reg_user_http(*l_rout_ptr);

  // 开始运行服务器
  auto l_listener = std::make_shared<http::http_listener>(g_thread().executor(), l_rout_ptr, l_args.port_);
  l_listener->run();
  l_save.run();
  in_vector.emplace_back(l_listener);

  return false;
}
}  // namespace doodle::launch