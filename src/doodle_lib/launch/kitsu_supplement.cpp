#include "kitsu_supplement.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/platform/win/register_file_type.h>
#include <doodle_core/sqlite_orm/detail/init_project.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/http/http_listener.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/core/scan_win_service.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/http_method/computing_time.h>
#include <doodle_lib/http_method/dingding_attendance.h>
#include <doodle_lib/http_method/file_association.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>
#include <doodle_lib/http_method/local_setting.h>
#include <doodle_lib/http_method/task_info.h>

#include <winreg/WinReg.hpp>
namespace doodle::launch {

struct kitsu_supplement_args_t {
  std::string kitsu_url_;
  std::uint16_t port_;
  FSys::path db_path_{};

  std::string kitsu_token_{};
  FSys::path kitsu_front_end_path_{};

  FSys::path kitsu_thumbnails_path_{};

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
    in_json.at("kitsu_front_end_path").get_to(out_obj.kitsu_front_end_path_);
    in_json.at("dingding_company_list").get_to(out_obj.dingding_company_list_);
    in_json.at("kitsu_thumbnails_path").get_to(out_obj.kitsu_thumbnails_path_);
  }
};

void get_register_info(kitsu_supplement_args_t& in_args) {
  try {
    winreg::RegKey l_key{};
    l_key.Open(
        HKEY_LOCAL_MACHINE, LR"(SOFTWARE\Doodle\MainConfig)", KEY_QUERY_VALUE | KEY_WOW64_64KEY | KEY_ENUMERATE_SUB_KEYS
    );
    for (auto&& l_sub : l_key.EnumSubKeys()) {
      winreg::RegKey l_sub_key{};
      l_sub_key.Open(
          HKEY_LOCAL_MACHINE, fmt::format(LR"(SOFTWARE\Doodle\MainConfig\{})", l_sub), KEY_QUERY_VALUE | KEY_WOW64_64KEY
      );
      auto l_id         = boost::lexical_cast<uuid>(conv::utf_to_utf<char>(l_sub_key.GetStringValue(L"id")));
      auto l_app_key    = conv::utf_to_utf<char>(l_sub_key.GetStringValue(L"app_key"));
      auto l_app_secret = conv::utf_to_utf<char>(l_sub_key.GetStringValue(L"app_secret"));
      auto l_name       = conv::utf_to_utf<char>(l_sub);

      in_args.dingding_company_list_.emplace_back(
          kitsu_supplement_args_t::dingding_company_t{
              .id_ = l_id, .app_key_ = l_app_key, .app_secret_ = l_app_secret, .name_ = l_name
          }
      );
    }
  } catch (...) {
    default_logger_raw()->log(log_loc(), level::err, boost::current_exception_diagnostic_information());
  }
}

bool kitsu_supplement_t::operator()(const argh::parser& in_arh, std::vector<std::shared_ptr<void>>& in_vector) {
  app_base::Get().use_multithread(true);
  kitsu_supplement_args_t l_args{
      .kitsu_url_ = "http://192.168.40.182",
      .port_      = 80,
      .db_path_   = "D:/kitsu.database",
      .kitsu_token_ =
          "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
          "eyJmcmVzaCI6ZmFsc2UsImlhdCI6MTcxNzU1MDUxMywianRpIjoiOTU0MDg1NjctMzE1OS00Y2MzLTljM2ItZmNiMzQ4MTIwNjU5IiwidHlw"
          "ZSI6ImFjY2VzcyIsInN1YiI6ImU5OWMyNjZhLTk1ZjUtNDJmNS1hYmUxLWI0MTlkMjk4MmFiMCIsIm5iZiI6MTcxNzU1MDUxMywiZXhwIjox"
          "NzY0NjMzNjAwLCJpZGVudGl0eV90eXBlIjoiYm90In0.xLV17bMK8VH0qavV4Ttbi43RhaBqpc1LtTUbRwu1684",
      .kitsu_front_end_path_  = "D:/kitsu/dist",
      .kitsu_thumbnails_path_ = "//192.168.10.242/TD_Data"
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

  if (in_arh["local"]) {
    if (auto l_str = in_arh({"port"}); l_str)
      l_args.port_ = boost::lexical_cast<std::uint16_t>(l_str.str());
    else
      l_args.port_ = 0;
    // 打开内存数据库
    g_ctx().emplace<sqlite_database>().load(":memory:");
    // 初始化路由
    auto l_rout_ptr = std::make_shared<http::http_route>();
    http::task_info_reg_local(*l_rout_ptr);
    http::local_setting_reg(*l_rout_ptr);
    // 开始运行服务器
    http::run_http_listener(g_io_context(), l_rout_ptr, l_args.port_);
    return false;
  }
  if (in_arh["epiboly"]) {
    if (auto l_str = in_arh({"port"}); l_str)
      l_args.port_ = boost::lexical_cast<std::uint16_t>(l_str.str());
    else
      l_args.port_ = 0;
    l_args.db_path_              = register_file_type::program_location().parent_path() / "epiboly.database";
    l_args.kitsu_front_end_path_ = register_file_type::program_location().parent_path() / "dist";
    // 打开内存数据库
    g_ctx().emplace<sqlite_database>().load(l_args.db_path_);
    // 初始化路由
    auto l_rout_ptr = http::create_kitsu_route(l_args.kitsu_front_end_path_);
    http::task_info_reg_local(*l_rout_ptr);
    http::local_setting_reg(*l_rout_ptr);
    // 开始运行服务器
    http::run_http_listener(g_io_context(), l_rout_ptr, l_args.port_);
    return false;
  }
  get_register_info(l_args);
  core_set::get_set().set_root(l_args.kitsu_thumbnails_path_);

  auto l_scan = g_ctx().emplace<std::shared_ptr<scan_win_service_t>>(std::make_shared<scan_win_service_t>());
  l_scan->use_cache();
  // 初始化数据库
  g_ctx().emplace<sqlite_database>().load(l_args.db_path_);

  // 初始化 ssl
  auto l_ssl_ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12_client);
  in_vector.emplace_back(l_ssl_ctx);

  {
    // 初始化 kitsu 客户端
    auto l_client = g_ctx().emplace<std::shared_ptr<kitsu::kitsu_client>>(
        std::make_shared<kitsu::kitsu_client>(g_io_context(), l_args.kitsu_url_)
    );
    l_client->set_access_token(std::string{l_args.kitsu_token_});
    g_ctx().emplace<http::kitsu_ctx_t>(
        l_args.kitsu_url_, l_args.kitsu_token_, l_args.kitsu_thumbnails_path_, l_args.kitsu_front_end_path_
    );

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

  if (in_arh["init"]) {
    return http::kitsu::init_context(), false;
  }

  l_scan->start();
  // 初始化路由
  auto l_rout_ptr = http::create_kitsu_route(l_args.kitsu_front_end_path_);
  http::reg_computing_time(*l_rout_ptr);
  http::reg_dingding_attendance(*l_rout_ptr);
  // 开始运行服务器
  http::run_http_listener(g_io_context(), l_rout_ptr, l_args.port_);

  return false;
}
} // namespace doodle::launch