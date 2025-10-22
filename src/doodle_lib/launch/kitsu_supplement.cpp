#include "kitsu_supplement.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/core/authorization.h>
#include <doodle_core/platform/win/register_file_type.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/http/http_listener.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/http_method/computing_time.h>
#include <doodle_lib/http_method/dingding_attendance.h>
#include <doodle_lib/http_method/file_association.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/seed_email.h>

#include <winreg/WinReg.hpp>
namespace doodle {

struct kitsu_supplement_args_t {
  std::string kitsu_url_;
  std::uint16_t port_;
  FSys::path db_path_{};

  std::string kitsu_token_{};
  FSys::path kitsu_front_end_path_{};

  FSys::path kitsu_thumbnails_path_{};
  std::vector<std::string> deepseek_keys_{};

  /// 即梦授权
  std::string ji_meng_access_key_id_;
  std::string ji_meng_secret_access_key_;
  /// 会话 jwt token
  std::string secret_;

  std::string domain_protocol_;
  std::string domain_name_;
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

  struct mail_config_t {
    std::string address_;
    std::uint32_t port_;

    std::string username_;
    std::string password_;

    friend void from_json(const nlohmann::json& in_json, mail_config_t& out_obj) {
      in_json.at("address").get_to(out_obj.address_);
      in_json.at("port").get_to(out_obj.port_);
      in_json.at("username").get_to(out_obj.username_);
      in_json.at("password").get_to(out_obj.password_);
    }
  };

  std::vector<dingding_company_t> dingding_company_list_{};

  mail_config_t mail_config_;

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
    in_json.at("deepseek_keys").get_to(out_obj.deepseek_keys_);
    if (in_json.contains("ji_meng_access_key_id"))
      in_json.at("ji_meng_access_key_id").get_to(out_obj.ji_meng_access_key_id_);
    if (in_json.contains("ji_meng_secret_access_key"))
      in_json.at("ji_meng_secret_access_key").get_to(out_obj.ji_meng_secret_access_key_);
    if (in_json.contains("mail_config")) in_json.at("mail_config").get_to(out_obj.mail_config_);
    if (in_json.contains("domain_protocol")) in_json.at("domain_protocol").get_to(out_obj.domain_protocol_);
    if (in_json.contains("domain_name")) in_json.at("domain_name").get_to(out_obj.domain_name_);
    if (in_json.contains("secret")) in_json.at("secret").get_to(out_obj.secret_);
  }
};

void get_register_info(kitsu_supplement_args_t& in_args) {
  try {
    if (winreg::RegKey l_key{}; l_key.TryOpen(
            HKEY_LOCAL_MACHINE, LR"(SOFTWARE\Doodle\MainConfig)",
            KEY_QUERY_VALUE | KEY_WOW64_64KEY | KEY_ENUMERATE_SUB_KEYS
        ))
      for (auto&& l_sub : l_key.EnumSubKeys()) {
        winreg::RegKey l_sub_key{};
        l_sub_key.Open(
            HKEY_LOCAL_MACHINE, fmt::format(LR"(SOFTWARE\Doodle\MainConfig\{})", l_sub),
            KEY_QUERY_VALUE | KEY_WOW64_64KEY
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
    else
      default_logger_raw()->error("无法打开键 SOFTWARE/Doodle/MainConfig");

    if (winreg::RegKey l_key{};
        l_key.TryOpen(HKEY_LOCAL_MACHINE, LR"(SOFTWARE\Doodle\MainConfig)", KEY_QUERY_VALUE | KEY_WOW64_64KEY).Failed())
      return default_logger_raw()->error("无法打开键 SOFTWARE/Doodle/MainConfig");
    else {
      auto l_value_w = l_key.GetMultiStringValue(L"Deepseek");
      in_args.deepseek_keys_ =
          l_value_w |
          ranges::views::transform([](const std::wstring& in) -> std::string { return conv::utf_to_utf<char>(in); }) |
          ranges::to_vector;

      if (auto l_value = l_key.TryGetStringValue(L"ji_meng_access_key_id"); l_value.IsValid())
        in_args.ji_meng_access_key_id_ = conv::utf_to_utf<char>(l_value.GetValue());
      if (auto l_value = l_key.TryGetStringValue(L"ji_meng_secret_access_key"); l_value.IsValid())
        in_args.ji_meng_secret_access_key_ = conv::utf_to_utf<char>(l_value.GetValue());
      if (auto l_value = l_key.TryGetStringValue(L"domain_name"); l_value.IsValid())
        in_args.domain_name_ = conv::utf_to_utf<char>(l_value.GetValue());
      if (auto l_value = l_key.TryGetStringValue(L"domain_protocol"); l_value.IsValid())
        in_args.domain_protocol_ = conv::utf_to_utf<char>(l_value.GetValue());
      if (auto l_value = l_key.TryGetStringValue(L"secret"); l_value.IsValid())
        in_args.secret_ = conv::utf_to_utf<char>(l_value.GetValue());
    }
    if (winreg::RegKey l_key{};
        l_key.TryOpen(HKEY_LOCAL_MACHINE, LR"(SOFTWARE\Doodle\Email)", KEY_QUERY_VALUE | KEY_WOW64_64KEY).Failed())
      default_logger_raw()->error("无法打开键 SOFTWARE/Doodle/Email");
    else {
      if (auto l_value = l_key.TryGetStringValue(L"address"); l_value.IsValid())
        in_args.mail_config_.address_ = conv::utf_to_utf<char>(l_value.GetValue());
      if (auto l_value = l_key.TryGetDwordValue(L"port"); l_value.IsValid())
        in_args.mail_config_.port_ = boost::numeric_cast<std::uint32_t>(l_value.GetValue());
      if (auto l_value = l_key.TryGetStringValue(L"username"); l_value.IsValid())
        in_args.mail_config_.username_ = conv::utf_to_utf<char>(l_value.GetValue());
      if (auto l_value = l_key.TryGetStringValue(L"password"); l_value.IsValid())
        in_args.mail_config_.password_ = conv::utf_to_utf<char>(l_value.GetValue());
    }
  } catch (...) {
    default_logger_raw()->log(log_loc(), level::err, boost::current_exception_diagnostic_information());
  }
}

bool kitsu_supplement_main::init() {
  app_base::Get().use_multithread(true);
  kitsu_supplement_args_t l_args{
      .kitsu_url_ = "http://192.168.40.182",
      .port_      = 80,
      .db_path_   = "D:/kitsu_new.database",
      .kitsu_token_ =
          "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
          "eyJmcmVzaCI6ZmFsc2UsImlhdCI6MTcxNzU1MDUxMywianRpIjoiOTU0MDg1NjctMzE1OS00Y2MzLTljM2ItZmNiMzQ4MTIwNjU5IiwidHlw"
          "ZSI6ImFjY2VzcyIsInN1YiI6ImU5OWMyNjZhLTk1ZjUtNDJmNS1hYmUxLWI0MTlkMjk4MmFiMCIsIm5iZiI6MTcxNzU1MDUxMywiZXhwIjox"
          "NzY0NjMzNjAwLCJpZGVudGl0eV90eXBlIjoiYm90In0.xLV17bMK8VH0qavV4Ttbi43RhaBqpc1LtTUbRwu1684",
      .kitsu_front_end_path_  = "D:/kitsu/dist",
      .kitsu_thumbnails_path_ = "D:/kitsu_data",
      .secret_                = "22T0iwSHK7qkhdI6",
      .domain_protocol_       = "http",
      .domain_name_           = "192.168.40.181",
  };

  if (auto l_file_path = arg_({"config"}); l_file_path) {
    auto l_json = nlohmann::json::parse(FSys::ifstream{FSys::from_quotation_marks(l_file_path.str())});
    try {
      l_json.get_to<kitsu_supplement_args_t>(l_args);
    } catch (...) {
      default_logger_raw()->log(log_loc(), level::err, boost::current_exception_diagnostic_information());
      return true;
    }
  }

  if (arg_["local"]) {
    if (auto l_str = arg_({"port"}); l_str)
      l_args.port_ = boost::lexical_cast<std::uint16_t>(l_str.str());
    else
      l_args.port_ = 0;
    core_set::get_set().set_root("D:/sy_maigc");
    // 打开内存数据库
    g_ctx().emplace<sqlite_database>().load(core_set::get_set().get_cache_root("database") / "kitsu.database");
    // 初始化授权上下文
    g_ctx().emplace<authorization>(core_set::get_set().authorize_);
    // 初始化路由
    auto l_rout_ptr = http::create_kitsu_local_route();
    // 开始运行服务器
    http::run_http_listener(g_io_context(), l_rout_ptr, l_args.port_);
    return true;
  }
  if (arg_["epiboly"]) {
    if (auto l_str = arg_({"port"}); l_str)
      l_args.port_ = boost::lexical_cast<std::uint16_t>(l_str.str());
    else
      l_args.port_ = 0;
    // 调整一些特有的上下文
    l_args.db_path_               = register_file_type::program_location().parent_path() / "epiboly.database";
    l_args.kitsu_front_end_path_  = register_file_type::program_location().parent_path() / "dist";
    l_args.kitsu_thumbnails_path_ = register_file_type::program_location().parent_path() / "thumbnails";
    // 初始化上下文
    g_ctx().emplace<http::kitsu_ctx_t>(
        l_args.kitsu_url_, l_args.kitsu_token_, l_args.kitsu_thumbnails_path_, l_args.kitsu_front_end_path_
    );
    // 打开内存数据库
    g_ctx().emplace<sqlite_database>().load(l_args.db_path_);
    // 初始化路由
    auto l_rout_ptr = http::create_kitsu_epiboly_route(l_args.kitsu_front_end_path_);
    // 开始运行服务器
    http::run_http_listener(g_io_context(), l_rout_ptr, l_args.port_);
    return true;
  }
  get_register_info(l_args);
  core_set::get_set().set_root(l_args.kitsu_thumbnails_path_);

  // 初始化数据库
  g_ctx().emplace<sqlite_database>().load(l_args.db_path_);

  // 初始化 ssl
  auto l_ssl_ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12_client);
  facets_.emplace_back(l_ssl_ctx);

  {
    g_ctx().emplace<http::kitsu_ctx_t>(
        l_args.kitsu_url_, l_args.kitsu_token_, l_args.kitsu_thumbnails_path_, l_args.kitsu_front_end_path_,
        l_args.deepseek_keys_, l_args.ji_meng_access_key_id_, l_args.ji_meng_secret_access_key_, l_args.secret_,
        l_args.domain_protocol_, l_args.domain_name_
    );
    if (!l_args.mail_config_.username_.empty() && !l_args.mail_config_.password_.empty())
      g_ctx().emplace<email::seed_email>(
          l_args.mail_config_.address_, l_args.mail_config_.port_, l_args.mail_config_.username_,
          l_args.mail_config_.password_
      );

    // 初始化钉钉客户端
    auto& l_d = g_ctx().emplace<dingding::dingding_company>();

    for (auto&& l_c : l_args.dingding_company_list_) {
      l_d.company_info_map_.emplace(
          l_c.id_, dingding::dingding_company::company_info{
                       .corp_id    = l_c.id_,
                       .app_key    = l_c.app_key_,
                       .app_secret = l_c.app_secret_,
                       .name       = l_c.name_,
                       .ctx_ptr    = l_ssl_ctx
                   }
      );
    }
  }
  // 初始化路由
  auto l_rout_ptr = http::create_kitsu_route_2(l_args.kitsu_front_end_path_);
  // 开始运行服务器
  http::run_http_listener(g_io_context(), l_rout_ptr, l_args.port_);

  return true;
}
}  // namespace doodle