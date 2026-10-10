//
// 签名 URL (url_sign_t) 测试
//
// 这份盯的是「把素材 URL 交给第三方去取图」的凭据: 短时效 + 绑定完整路径 + 只 GET。
// 全是纯函数, 不需要 app_base / 数据库 / 网络。
//

#include <doodle_lib/http_client/url_sign.h>

#include <boost/test/unit_test.hpp>

#include <map>
#include <string>
#include <string_view>

BOOST_AUTO_TEST_SUITE(url_sign)

namespace {

using namespace doodle::http;

const std::string g_secret{"test-secret"};
const std::string g_uid{"c9f2b8c0-4a1d-4e3b-9f5c-7d6e0a2f3b8d"};
const std::string g_path{"/api/seedance2/pictures/c9f2b8c0-4a1d-4e3b-9f5c-7d6e0a2f3b8d.png"};

/// 把 "path?a=1&b=2" 解析成键值对(测试自己解析, 不依赖被测实现)
std::map<std::string, std::string> parse_query(std::string_view in_url) {
  std::map<std::string, std::string> l_ret{};
  const auto l_pos = in_url.find('?');
  if (l_pos == std::string_view::npos) return l_ret;
  auto l_query = in_url.substr(l_pos + 1);
  while (!l_query.empty()) {
    const auto l_amp = l_query.find('&');
    const auto l_one = l_query.substr(0, l_amp);
    if (const auto l_eq = l_one.find('='); l_eq != std::string_view::npos)
      l_ret[std::string{l_one.substr(0, l_eq)}] = std::string{l_one.substr(l_eq + 1)};
    if (l_amp == std::string_view::npos) break;
    l_query = l_query.substr(l_amp + 1);
  }
  return l_ret;
}

/// 取 URL 的路径部分(去掉 scheme://host:port 与查询串)
std::string url_path(std::string_view in_url) {
  const auto l_end_of_query = in_url.find('?');
  const auto l_after_scheme = in_url.find("://");
  if (l_after_scheme == std::string_view::npos) return std::string{in_url.substr(0, l_end_of_query)};
  const auto l_slash = in_url.find('/', l_after_scheme + 3);
  if (l_slash == std::string_view::npos) return {};
  const auto l_len = l_end_of_query == std::string_view::npos ? std::string_view::npos : l_end_of_query - l_slash;
  return std::string{in_url.substr(l_slash, l_len)};
}

}  // namespace

// ---------------------------------------------------------------------------
// 形态: 相对路径 + exp/uid/sig
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(sign_path_stays_relative_and_carries_exp_uid_sig) {
  const auto l_signed = url_sign_t::sign_path(g_path, g_uid, g_secret);
  const auto l_query  = parse_query(l_signed);

  // 必须保持「站点内路径」形态: 调用方(add_ip_to_req) 靠这个前缀补公网 IP,
  // 一旦这里返回绝对 URL, 补前缀逻辑就会失效。
  BOOST_CHECK_EQUAL(l_signed.starts_with("/api/seedance2/pictures/"), true);
  BOOST_CHECK_EQUAL(l_signed.starts_with("http"), false);

  BOOST_CHECK_EQUAL(l_query.count(url_sign_t::g_exp_key.data()), 1);
  BOOST_CHECK_EQUAL(l_query.count(url_sign_t::g_uid_key.data()), 1);
  BOOST_CHECK_EQUAL(l_query.count(url_sign_t::g_sig_key.data()), 1);
  BOOST_CHECK_EQUAL(l_query.at(url_sign_t::g_uid_key.data()), g_uid);
  BOOST_CHECK_EQUAL(url_path(l_signed), g_path);
}

BOOST_AUTO_TEST_CASE(sign_path_is_idempotent_for_already_signed_path) {
  const auto l_signed = url_sign_t::sign_path(g_path, g_uid, g_secret);
  // 重复签名会让签名内容与校验时的规范串不一致, 必须原样返回
  BOOST_CHECK_EQUAL(url_sign_t::sign_path(l_signed, g_uid, g_secret), l_signed);
}

BOOST_AUTO_TEST_CASE(sign_path_keeps_existing_query_with_ampersand) {
  const auto l_signed = url_sign_t::sign_path("/api/x.png?v=2", g_uid, g_secret);
  BOOST_CHECK_EQUAL(l_signed.starts_with("/api/x.png?v=2&exp="), true);
  // 参与签名的只有路径部分, 查询串不进签名
  const auto l_query = parse_query(l_signed);
  BOOST_CHECK_EQUAL(
      url_sign_t::verify(
          "/api/x.png", l_query.at(url_sign_t::g_exp_key.data()), l_query.at(url_sign_t::g_uid_key.data()),
          l_query.at(url_sign_t::g_sig_key.data()), g_secret, url_sign_t::now_seconds()
      ),
      true
  );
}

BOOST_AUTO_TEST_CASE(sign_path_returns_empty_for_empty_path) {
  BOOST_CHECK_EQUAL(url_sign_t::sign_path("", g_uid, g_secret), "");
}

// ---------------------------------------------------------------------------
// 校验
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(verify_accepts_a_freshly_signed_path) {
  const auto l_signed = url_sign_t::sign_path(g_path, g_uid, g_secret);
  const auto l_query  = parse_query(l_signed);

  std::string l_reason{};
  const bool l_ok = url_sign_t::verify(
      g_path, l_query.at(url_sign_t::g_exp_key.data()), l_query.at(url_sign_t::g_uid_key.data()),
      l_query.at(url_sign_t::g_sig_key.data()), g_secret, url_sign_t::now_seconds(), &l_reason
  );
  BOOST_CHECK_EQUAL(l_ok, true);
  BOOST_CHECK_EQUAL(l_reason, "");
}

BOOST_AUTO_TEST_CASE(verify_rejects_tampered_path_uid_exp_and_secret) {
  const auto l_signed = url_sign_t::sign_path(g_path, g_uid, g_secret);
  const auto l_query  = parse_query(l_signed);
  const auto l_exp    = l_query.at(url_sign_t::g_exp_key.data());
  const auto l_uid    = l_query.at(url_sign_t::g_uid_key.data());
  const auto l_sig    = l_query.at(url_sign_t::g_sig_key.data());
  const auto l_now    = url_sign_t::now_seconds();

  // 换一条路径(拿 A 图的签名去取 B 图) —— 签名绑定完整路径, 必须失败
  BOOST_CHECK_EQUAL(
      url_sign_t::verify(
          "/api/seedance2/pictures/00000000-0000-0000-0000-000000000000.png", l_exp, l_uid, l_sig, g_secret, l_now
      ),
      false
  );
  // 换一个人(uid 参与签名)
  BOOST_CHECK_EQUAL(url_sign_t::verify(g_path, l_exp, "other-uid", l_sig, g_secret, l_now), false);
  // 改过期时间(等于无限延长有效期)
  BOOST_CHECK_EQUAL(url_sign_t::verify(g_path, std::to_string(l_now + 100000), l_uid, l_sig, g_secret, l_now), false);
  // 换密钥
  BOOST_CHECK_EQUAL(url_sign_t::verify(g_path, l_exp, l_uid, l_sig, "another-secret", l_now), false);
}

BOOST_AUTO_TEST_CASE(verify_rejects_expired_signature) {
  // 直接构造一个已过期的签名: 用 in_exp = 过去时间
  const auto l_expired_exp = url_sign_t::now_seconds() - 10;
  const auto l_sig         = url_sign_t::sign(g_path, l_expired_exp, g_uid, g_secret);

  std::string l_reason{};
  BOOST_CHECK_EQUAL(
      url_sign_t::verify(
          g_path, std::to_string(l_expired_exp), g_uid, l_sig, g_secret, url_sign_t::now_seconds(), &l_reason
      ),
      false
  );
  // 过期的失败原因必须是「过期」而不是「签名不匹配」: 两者的排查方向完全不同
  BOOST_CHECK_EQUAL(l_reason, "签名已过期");
}

BOOST_AUTO_TEST_CASE(verify_rejects_missing_or_malformed_parts) {
  const auto l_signed = url_sign_t::sign_path(g_path, g_uid, g_secret);
  const auto l_query  = parse_query(l_signed);
  const auto l_exp    = l_query.at(url_sign_t::g_exp_key.data());
  const auto l_sig    = l_query.at(url_sign_t::g_sig_key.data());
  const auto l_now    = url_sign_t::now_seconds();

  BOOST_CHECK_EQUAL(url_sign_t::verify("", l_exp, g_uid, l_sig, g_secret, l_now), false);           // 空路径
  BOOST_CHECK_EQUAL(url_sign_t::verify(g_path, "", g_uid, l_sig, g_secret, l_now), false);          // 缺 exp
  BOOST_CHECK_EQUAL(url_sign_t::verify(g_path, l_exp, g_uid, "", g_secret, l_now), false);          // 缺 sig
  BOOST_CHECK_EQUAL(url_sign_t::verify(g_path, "abc", g_uid, l_sig, g_secret, l_now), false);       // exp 非数字
  BOOST_CHECK_EQUAL(url_sign_t::verify(g_path, l_exp, g_uid, "deadbeef", g_secret, l_now), false);  // 长度不符
}

BOOST_AUTO_TEST_CASE(verify_is_case_insensitive_on_hex_signature) {
  const auto l_signed = url_sign_t::sign_path(g_path, g_uid, g_secret);
  const auto l_query  = parse_query(l_signed);

  // 第三方转发时可能改动大小写, 不该因此失效
  std::string l_upper = l_query.at(url_sign_t::g_sig_key.data());
  for (auto& l_ch : l_upper) l_ch = static_cast<char>(std::toupper(static_cast<unsigned char>(l_ch)));

  BOOST_CHECK_EQUAL(
      url_sign_t::verify(
          g_path, l_query.at(url_sign_t::g_exp_key.data()), g_uid, l_upper, g_secret, url_sign_t::now_seconds()
      ),
      true
  );
}

// ---------------------------------------------------------------------------
// 规范串与解析
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(canonical_is_path_newline_exp_newline_uid) {
  // 规范串的形状是签名稳定性的根: 换行分隔 + 三个字段都参与
  BOOST_CHECK_EQUAL(url_sign_t::canonical("/a.png", 1700000000, "u1"), "/a.png\n1700000000\nu1");
  BOOST_CHECK_EQUAL(
      url_sign_t::canonical("/a.png", 1700000000, "u1") == url_sign_t::canonical("/a.png", 1700000000, "u2"), false
  );
}

BOOST_AUTO_TEST_CASE(parse_exp_only_accepts_decimal_digits) {
  const auto l_value = url_sign_t::parse_exp("1700000000");
  BOOST_REQUIRE(l_value.has_value());
  BOOST_CHECK_EQUAL(*l_value, 1700000000);
  BOOST_CHECK(!url_sign_t::parse_exp("").has_value());
  BOOST_CHECK(!url_sign_t::parse_exp("-1").has_value());
  BOOST_CHECK(!url_sign_t::parse_exp("1.5").has_value());
  BOOST_CHECK(!url_sign_t::parse_exp("0x10").has_value());
  BOOST_CHECK(!url_sign_t::parse_exp("17 00").has_value());
  BOOST_CHECK(!url_sign_t::parse_exp(" 1700000000").has_value());
}

BOOST_AUTO_TEST_SUITE_END()
