//
// Created by TD on 24-8-20.
//

#pragma once
#include <doodle_core/core/http_client_core.h>

#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/scan_assets/base.h>
namespace doodle::http {
class http_route;
using http_route_ptr = std::shared_ptr<http_route>;

struct kitsu_ctx_t {
  std::string url_;
  std::string access_token_;
  /// 产生的资产储存位置
  std::filesystem::path root_;
  /// 前端部署的问价所在位置
  std::filesystem::path front_end_root_;
  /// deepseek ai 的key
  std::vector<std::string> deepseek_keys_;

  /// 即梦授权
  std::string ji_meng_access_key_id_;
  std::string ji_meng_secret_access_key_;

  /// 会话 jwt token
  std::string secret_;
  /// 服务器 协议和域名(基本在发送电子邮件时使用)
  std::string domain_protocol_;
  std::string domain_name_;
};

http_route_ptr create_kitsu_route_2(const FSys::path& in_root);
http_route_ptr create_kitsu_epiboly_route(const FSys::path& in_root);
http_route_ptr create_kitsu_local_route();

namespace kitsu {

doodle::details::assets_type_enum conv_assets_type_enum(const std::string& in_name);

uuid get_url_project_id(const boost::urls::url& in_url);

std::string_view mime_type(const FSys::path& in_ext);

}  // namespace kitsu
void preview_reg(http_route& in_http_route);

}  // namespace doodle::http