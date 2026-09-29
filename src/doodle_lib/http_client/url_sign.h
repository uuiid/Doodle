//
// Created by TD on 2026/02/11.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

namespace doodle::http {

/**
 * 站点内取图 URL 的签名工具。
 *
 * 为什么需要: 把素材 URL 交给第三方(火山方舟)去取图时, 对方无法携带我们的会话 cookie,
 * 而会话 JWT 是长效的(cookie 365 天)且服务端没有吊销名单, 不能放进 URL。签名 URL 提供的是
 * 「短时效 + 绑定完整路径与人 + 只允许 GET」的能力: 泄漏出去的签名最多让第三方在所绑定的
 * 那一条路径上、以指定人员的身份读一张图, 且很快过期。
 *
 * 形态:
 *   <path>?exp=<unix 秒>&uid=<人员 uuid>&sig=<hex(hmac_sha256(secret, "<path>\n<exp>\n<uid>"))>
 *
 * uid 参与签名并且在服务端会**真正加载该人员**(与 cookie 路径完全一样, 之后 handler 里的
 * check_project_access 等校验照常生效), 所以签名不是绕过授权, 而是换一种凭据。
 * uid 允许为空(没有人员上下文的调用方), 此时不加载 person_, 需要人员校验的 handler 会
 * 按「空人员」fail-closed。
 *
 * 约定:
 *   - 待签名的 path 是**不含查询串**的路径部分(与 boost::url::path() 一致);
 *   - sign_path() 返回的仍是**站点内相对路径**(不以 http 开头), 交由
 *     seedance2_client::add_ip_to_req() 补公网前缀, 因此这里绝不能返回绝对 URL;
 *   - 校验入口见 http_jwt_or_signed_fun::parse_header(), 只对 GET 生效。
 */
struct DOODLELIB_API url_sign_t {
  /// 查询串里的过期时间(秒, unix 时间戳)
  constexpr static std::string_view g_exp_key{"exp"};
  /// 查询串里的人员 uuid(可为空串)
  constexpr static std::string_view g_uid_key{"uid"};
  /// 查询串里的签名(小写 hex)
  constexpr static std::string_view g_sig_key{"sig"};
  /// 默认有效期: 十分钟。第三方取图是即时动作, 留足重试余量即可。
  constexpr static std::chrono::seconds g_default_ttl{std::chrono::minutes{10}};

  /// 当前 unix 秒
  static std::int64_t now_seconds();
  /// 待签名的规范串: "<path>\n<exp>\n<uid>"
  static std::string canonical(std::string_view in_path, std::int64_t in_exp, std::string_view in_uid);
  /// hex(hmac_sha256(in_secret, canonical(in_path, in_exp, in_uid))), 小写
  static std::string sign(
      std::string_view in_path, std::int64_t in_exp, std::string_view in_uid, std::string_view in_secret
  );

  /**
   * 给站点内路径追加签名参数, 返回**相对路径**。
   * - in_path 可以带查询串, 但**只有路径部分**参与签名;
   * - 已经带 sig 参数的原样返回(不重复签名);
   * - 空路径原样返回。
   */
  static std::string sign_path(
      std::string_view in_path, std::string_view in_uid, std::string_view in_secret,
      std::chrono::seconds in_ttl = g_default_ttl
  );

  /// 解析查询串里的过期时间(纯十进制, 不做异常)
  static bool parse_exp(std::string_view in_exp, std::int64_t& out_exp);

  /**
   * 校验签名。in_path 为**不含查询串**的路径部分; in_uid 可为空串; in_now 由调用方传入
   * (便于测试)。失败原因写入 out_reason(可为空)。
   */
  static bool verify(
      std::string_view in_path, std::string_view in_exp, std::string_view in_uid, std::string_view in_sig,
      std::string_view in_secret, std::int64_t in_now, std::string* out_reason = nullptr
  );
};

}  // namespace doodle::http
