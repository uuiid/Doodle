//
// Created by TD on 2026/02/11.
//

#include <doodle_lib/http_client/url_sign.h>

#include <cctype>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <cryptopp/hmac.h>
#include <cryptopp/sha.h>
#include <fmt/format.h>

namespace doodle::http {

std::int64_t url_sign_t::now_seconds() {
  return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string url_sign_t::canonical(std::string_view in_path, std::int64_t in_exp, std::string_view in_uid) {
  return fmt::format("{}\n{}\n{}", in_path, in_exp, in_uid);
}

std::string url_sign_t::sign(
    std::string_view in_path, std::int64_t in_exp, std::string_view in_uid, std::string_view in_secret
) {
  CryptoPP::HMAC<CryptoPP::SHA256> l_hmac{reinterpret_cast<const CryptoPP::byte*>(in_secret.data()), in_secret.size()};
  std::string l_sig{};
  const auto l_canonical = canonical(in_path, in_exp, in_uid);
  // 第二个参数 false: 统一输出小写 hex(CryptoPP 默认大写)
  CryptoPP::StringSource{
      l_canonical, true,
      new CryptoPP::HashFilter{l_hmac, new CryptoPP::HexEncoder{new CryptoPP::StringSink{l_sig}, false}}
  };
  return l_sig;
}

std::string url_sign_t::sign_path(
    std::string_view in_path, std::string_view in_uid, std::string_view in_secret, std::chrono::seconds in_ttl
) {
  if (in_path.empty()) return std::string{in_path};
  if (in_path.find(fmt::format("{}=", g_sig_key)) != std::string_view::npos) return std::string{in_path};

  const auto l_path = in_path.substr(0, in_path.find('?'));
  const auto l_exp  = now_seconds() + in_ttl.count();
  return fmt::format(
      "{}{}{}={}&{}={}&{}={}", in_path, in_path.find('?') == std::string_view::npos ? "?" : "&", g_exp_key, l_exp,
      g_uid_key, in_uid, g_sig_key, sign(l_path, l_exp, in_uid, in_secret)
  );
}

std::optional<std::int64_t> url_sign_t::parse_exp(std::string_view in_exp) {
  if (in_exp.empty()) return std::nullopt;
  std::int64_t l_value{};
  for (const auto l_ch : in_exp) {
    if (l_ch < '0' || l_ch > '9') return std::nullopt;
    l_value = l_value * 10 + (l_ch - '0');
    if (l_value < 0) return std::nullopt;  // 溢出
  }
  return l_value;
}

bool url_sign_t::verify(
    std::string_view in_path, std::string_view in_exp, std::string_view in_uid, std::string_view in_sig,
    std::string_view in_secret, std::int64_t in_now, std::string* out_reason
) {
  const auto l_fail = [&](std::string_view in_reason) {
    if (out_reason != nullptr) *out_reason = std::string{in_reason};
    return false;
  };

  if (in_path.empty()) return l_fail("路径为空");
  if (in_exp.empty() || in_sig.empty()) return l_fail("缺少 exp/sig 参数");

  const auto l_exp = parse_exp(in_exp);
  if (!l_exp) return l_fail("exp 不是合法的 unix 时间戳");
  if (*l_exp < in_now) return l_fail("签名已过期");

  const auto l_expect = sign(in_path, *l_exp, in_uid, in_secret);
  if (l_expect.size() != in_sig.size()) return l_fail("签名长度不符");

  // 大小写不敏感比较: 第三方可能改动大小写, 但不该因此失效
  for (std::size_t i = 0; i < l_expect.size(); ++i)
    if (static_cast<char>(std::tolower(static_cast<unsigned char>(l_expect[i]))) !=
        static_cast<char>(std::tolower(static_cast<unsigned char>(in_sig[i]))))
      return l_fail("签名不匹配");

  return true;
}

}  // namespace doodle::http
