#include "kitsu_supplement.h"

#include <doodle_core/sqlite_orm/detail/init_project.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_app/app/app_command.h>

#include <doodle_lib/core/http/http_listener.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/core/scan_win_service.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/http_method/computing_time.h>
#include <doodle_lib/http_method/dingding_attendance.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>
#include <doodle_lib/http_method/sqlite/kitsu_backend_sqlite.h>
#include <doodle_lib/http_method/user_http.h>
namespace doodle::launch {
struct kitsu_supplement_args_t {
  std::string kitsu_url_;
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
    if (in_json.contains("kitsu_ip") && in_json.contains("kitsu_port")) {
      out_obj.kitsu_url_ = fmt::format(
          "http://{}:{}", in_json.at("kitsu_ip").get<std::string>(), in_json.at("kitsu_port").get<std::string>()
      );
    }
    if (in_json.contains("kitsu_url")) in_json.at("kitsu_url").get_to(out_obj.kitsu_url_);
    in_json.at("kitsu_token").get_to(out_obj.kitsu_token_);
    in_json.at("port").get_to(out_obj.port_);
    in_json.at("db_path").get_to(out_obj.db_path_);
    in_json.at("dingding_company_list").get_to(out_obj.dingding_company_list_);
  }
};

bool kitsu_supplement_t::operator()(const argh::parser& in_arh, std::vector<std::shared_ptr<void>>& in_vector) {
  app_base::Get().use_multithread(true);
  auto l_scan = g_ctx().emplace<std::shared_ptr<scan_win_service_t>>(std::make_shared<scan_win_service_t>());

  kitsu_supplement_args_t l_args{
      .kitsu_url_ = "http://192.168.40.182", .port_ = 50025, .db_path_ = "D:/kitsu.database"
  };

  if (auto l_file_path = in_arh({"config"}); l_file_path) {
    auto l_json = nlohmann::json::parse(FSys::ifstream{FSys::from_quotation_marks(l_file_path.str())});
    try {
      l_args = l_json.get<kitsu_supplement_args_t>();
    } catch (...) {
      default_logger_raw()->log(log_loc(), level::err, boost::current_exception_diagnostic_information());
      return true;
    }
  }

  // 初始化 ssl
  auto l_ssl_ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12_client);
  in_vector.emplace_back(l_ssl_ctx);

  // 初始化数据库
  {
    g_ctx().emplace<sqlite_database>().load(l_args.db_path_);
    doodle::details::init_project();
  }
  {
    // 初始化 kitsu 客户端
    auto l_client = g_ctx().emplace<std::shared_ptr<kitsu::kitsu_client>>(
        std::make_shared<kitsu::kitsu_client>(g_io_context(), l_args.kitsu_url_)
    );
    l_client->set_access_token(std::string{l_args.kitsu_token_});
    g_ctx().emplace<http::kitsu_ctx_t>(l_args.kitsu_url_, l_args.kitsu_token_);

    // 初始化钉钉客户端
    auto& l_d = g_ctx().emplace<dingding::dingding_company>();

    for (auto&& l_c : l_args.dingding_company_list_) {
      l_d.company_info_map_
          .emplace(
              l_c.id_,
              dingding::dingding_company::company_info{
                  .corp_id    = l_c.id_,
                  .app_key    = l_c.app_key_,
                  .app_secret = l_c.app_secret_,
                  .name       = l_c.name_,
                  .client_ptr = std::make_shared<dingding::client>(*l_ssl_ctx)
              }
          )
          .first->second.client_ptr->access_token(l_c.app_key_, l_c.app_secret_);
    }
  }
  l_scan->start();
  // 初始化路由
  auto l_rout_ptr = http::create_kitsu_route();
  http::reg_computing_time(*l_rout_ptr);
  http::reg_dingding_attendance(*l_rout_ptr);
  http::reg_user_http(*l_rout_ptr);

  // 开始运行服务器
  http::run_http_listener(g_io_context(), l_rout_ptr, l_args.port_);

  return false;
}
} // namespace doodle::launch