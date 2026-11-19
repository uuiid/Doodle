#include "kitsu_supplement_test.h"

#include "test_reg.h"

#include <doodle_core/metadata/kitsu_ctx_t.h>

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/crashpad.h>
#include <doodle_lib/core/http/http_listener.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/exe_warp/ue_exe.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/dll.hpp>

#include <opencv2/core/utility.hpp>
#include <windows.h>

namespace doodle {

namespace {

std::string arg_str(const argh::parser& in_arg, const std::string& in_name, const std::string& in_default) {
  if (auto l_str = in_arg({in_name}); l_str) return l_str.str();
  return in_default;
}

}  // namespace

bool kitsu_supplement_test_main::init() {
  app_base::Get().use_multithread(true);
  crashpad_init::get();

  auto& l_set     = core_set::get_set();
  l_set.maya_ctx_ = std::make_shared<maya_ctx>();
  l_set.ue_ctx_   = std::make_shared<ue_ctx>();
  l_set.database_ = std::make_shared<sqlite_storage>();

  auto l_root  = FSys::from_quotation_marks(arg_str(arg_, "root", (boost::dll::program_location().parent_path() / "test_data").generic_string()));
  auto l_db    = arg_str(arg_, "dbPath", ":memory:");
  auto l_secret = arg_str(arg_, "secret", "22T0iwSHK7qkhdI6");
  auto l_protocol = arg_str(arg_, "domain_protocol", "http");
  auto l_domain   = arg_str(arg_, "domain_name", "127.0.0.1");
  auto l_proxy    = arg_str(arg_, "http_proxy_url", "");
  auto l_port     = boost::numeric_cast<std::uint16_t>(std::stoi(arg_str(arg_, "port", "50140")));

  l_set.set_root(l_root);
  l_set.public_url_port_ = l_port;
  l_set.http_proxy_url_  = l_proxy;

  l_set.database_->open(FSys::path{l_db});
  l_set.database_->upgrade();

  auto l_ssl_ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12_client);
  facets_.emplace_back(l_ssl_ctx);
  l_set.ctx_ptr = l_ssl_ctx;

  l_set.kitsu_ctx_ = std::make_shared<http::kitsu_ctx_t>(l_root, l_secret, l_protocol, l_domain);

  cv::setNumThreads(::GetActiveProcessorCount(0));

  auto l_route_ptr = http::create_test_route(l_root.generic_string(), l_db, l_port);

  if (arg_["selftest"]) return true;

  http::run_http_listener(g_io_context(), l_route_ptr, l_port);
  default_logger_raw()->info(
      "kitsu_supplement_test_main listening :{} root={} db={}", l_port, l_root.generic_string(), l_db
  );
  return true;
}

}  // namespace doodle
