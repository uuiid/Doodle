#include "ai_client_base.h"

#include <doodle_core/exception/exception.h>

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_client/url_sign.h>
#include <doodle_lib/lib_warp/boost_fmt_beast.h>

#include <boost/url.hpp>
#include <fmt/format.h>

#include <chrono>

namespace doodle::http::seedance2 {

boost::asio::awaitable<ai_client_base::run_task_result_t> ai_client_base::run_task(
    const doodle::seedance2::task& in_task
) {
  // 提交阶段: 交给外部服务就返回, 不看结果 —— 异步后端此时只拿到 queued + task_id_,
  // 同步后端(推理 / API易 图片)则一次调用就是终态, 两种情况都由调用方按 status_ 处理。
  if (in_task.status_ == doodle::seedance2::task_status::preparing) co_return co_await submit_task(in_task);

  // 查询阶段: 已经提交过了, 只问一次进度。中间态(running/queued)原样返回, 由调用方的
  // 定时器下一轮再问; 查得成功才下载 —— 失败/取消等终态没有文件可取。
  auto l_result     = co_await query_task(in_task);
  l_result.task_id_ = in_task.task_id_;
  if (l_result.status_ == doodle::seedance2::task_status::succeeded) co_await download_result(l_result);
  co_return l_result;
}

boost::asio::awaitable<ai_client_base::run_task_result_t> ai_client_base::run_infer_task(
    const doodle::seedance2::task& in_task
) {
  throw_exception(
      doodle_error{
          boost::beast::http::status::not_implemented,
          fmt::format("后端不支持推理任务(任务 {})", boost::uuids::to_string(in_task.uuid_id_))
      }
  );
  co_return run_task_result_t{};
}

std::string ai_client_base::to_public_url(
    std::string_view in_url, std::string_view in_ip, std::string_view in_secret, std::string_view in_uid
) {
  if (in_url.empty()) return std::string{in_url};
  // 绝对 URL 原样保留: 对象存储的签名 URL、方舟自己的 URL 都自带完整鉴权信息,
  // 再补前缀或再签名只会把它弄坏。
  if (in_url.starts_with("http://") || in_url.starts_with("https://")) return std::string{in_url};
  // 其它带 scheme 的 URL 同样原样保留: data: 内联 base64(中转站的 images[] 就支持)、blob: 等
  // 都不是「站点内路径」, 补前缀会拼成 http://ip:{port}data:... 这种废 URL。
  if (const auto l_colon = in_url.find(':'); l_colon != std::string_view::npos && in_url.find('/') > l_colon)
    return std::string{in_url};

  // 站点内路径: 先签名(返回的仍是相对路径), 再补公网前缀
  auto l_path = std::string{in_url};
  if (!in_secret.empty()) l_path = url_sign_t::sign_path(l_path, in_uid, in_secret);
  return fmt::format("http://{}:{}{}", in_ip, core_set::get_set().public_url_port_, l_path);
}

nlohmann::json ai_client_base::add_ip_to_req(
    const nlohmann::json& in_req, std::string_view in_ip, std::string_view in_secret, std::string_view in_uid
) {
  nlohmann::json l_req = in_req;
  for_each_media_url(l_req, [&](nlohmann::json& in_url) {
    if (!in_url.is_string()) return;
    in_url = to_public_url(in_url.get<std::string>(), in_ip, in_secret, in_uid);
  });
  return l_req;
}

boost::asio::awaitable<std::string> ai_client_base::get_ip_str() {
  using http_client_ssl = doodle::http::http_client_ssl;

  constexpr auto g_ip_cache_ttl = std::chrono::seconds{1};

  if (!cached_ip_.empty() && std::chrono::steady_clock::now() < cached_ip_expire_) co_return cached_ip_;

  auto l_client = std::make_shared<http_client_ssl>("https://api.ip.sb", *core_set::get_set().ctx_ptr);
  boost::beast::http::request<boost::beast::http::empty_body> l_req{boost::beast::http::verb::get, "/ip", 11};
  l_req.set(boost::beast::http::field::host, l_client->server_ip_and_port_);
  l_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

  for (int i = 0; i < 3; ++i) {
    boost::beast::http::response<boost::beast::http::string_body> l_res{};
    try {
      co_await l_client->read_and_write(l_req, l_res, boost::asio::use_awaitable);
      if (l_res.result() != boost::beast::http::status::ok)
        throw_exception(doodle_error{"get_ip_str error {} {}", l_res.result(), l_res.body()});
      auto l_ip = l_res.body();
      if (l_ip.ends_with('\n')) l_ip.pop_back();
      cached_ip_        = l_ip;
      cached_ip_expire_ = std::chrono::steady_clock::now() + g_ip_cache_ttl;
      co_return l_ip;
    } catch (const boost::system::system_error& e) {
      if (e.code() == boost::asio::error::operation_aborted) throw;
      SPDLOG_LOGGER_ERROR(logger_, "get_ip_str error: {}", e.what());
      if (i == 2) throw;
    } catch (...) {
      auto l_err_str = boost::current_exception_diagnostic_information();
      SPDLOG_LOGGER_ERROR(logger_, "get_ip_str error: {}", l_err_str);
      if (i == 2) throw;
    }
  }
  co_return std::string{};  // unreachable
}

boost::asio::awaitable<FSys::path> ai_client_base::download_raw_file(std::string_view in_file_url) {
  using http_client_ssl = doodle::http::http_client_ssl;

  boost::urls::url l_url{in_file_url};
  auto l_url_path = std::string{l_url.encoded_path()};
  if (l_url_path.empty()) l_url_path = "/";
  auto l_ext        = FSys::path{l_url_path}.extension();
  // 请求行必须带上查询串: 火山引擎等对象存储的签名 URL 把鉴权信息放在 query 里, 丢掉会 403
  auto l_url_target = std::string{l_url.encoded_target()};
  if (l_url_target.empty() || l_url_target.front() != '/') l_url_target = l_url_path + l_url_target;
  auto l_client = std::make_shared<http_client_ssl>(std::string{in_file_url}, *core_set::get_set().ctx_ptr);

  boost::beast::http::request<boost::beast::http::empty_body> req{boost::beast::http::verb::get, l_url_target, 11};
  req.set(boost::beast::http::field::accept, "*/*");
  req.set(boost::beast::http::field::host, l_client->server_ip_);
  req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  req.keep_alive(true);
  l_client->body_limit_ = 1024 * 1024 * 1024;  // 1GB
  l_client->set_timeout(1200s);

  FSys::path l_path;
  for (int i = 0; i < 3; ++i) try {
      boost::beast::http::response<boost::beast::http::file_body> l_res{};
      l_path = core_set::get_set().get_cache_root("http") /
               (core_set::get_set().get_uuid_str() + l_ext.generic_string());
      boost::system::error_code l_ec{};
      l_res.body().open(l_path.generic_string().c_str(), boost::beast::file_mode::write, l_ec);
      if (l_ec) throw_exception(http_request_error{boost::beast::http::status::internal_server_error, l_ec.message()});
      co_await l_client->read_and_write(req, l_res, boost::asio::use_awaitable);
      DOODLE_CHICK(l_res.result() == boost::beast::http::status::ok, "download_result error: {}", l_res.result());
      break;
    } catch (const std::exception& e) {
      logger_->error("download_result error: {}, retrying {}/3", e.what(), i + 1);
      if (i == 2) throw_exception(http_request_error{boost::beast::http::status::internal_server_error, e.what()});
    }

  co_return l_path;
}

}  // namespace doodle::http::seedance2