#include "ai_client_base.h"

#include <doodle_core/exception/exception.h>

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/lib_warp/boost_fmt_beast.h>

#include <boost/url.hpp>
#include <fmt/format.h>

namespace doodle::http::seedance2 {

boost::asio::awaitable<std::string> ai_client_base::get_ip_str() {
  using http_client_ssl = doodle::http::http_client_ssl;
  auto l_client         = std::make_shared<http_client_ssl>("https://api.ip.sb", *core_set::get_set().ctx_ptr);
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
  auto l_ext      = FSys::path{l_url_path}.extension();
  auto l_client   = std::make_shared<http_client_ssl>(std::string{in_file_url}, *core_set::get_set().ctx_ptr);

  boost::beast::http::request<boost::beast::http::empty_body> req{boost::beast::http::verb::get, l_url_path, 11};
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

boost::asio::awaitable<void> ai_client_base::query_task_result_t::download() {
  DOODLE_CHICK(client_ptr_, "client_ptr_ is null");
  return client_ptr_->download_result(this);
}

}  // namespace doodle::http::seedance2