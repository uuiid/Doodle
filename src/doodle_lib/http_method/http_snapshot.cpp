//
// Created by TD on 2024/3/14.
//

#include "http_snapshot.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/server_task_info.h>
#include <doodle_core/platform/win/register_file_type.h>
#include <doodle_core/sqlite_orm/sqlite_snapshot.h>
namespace doodle::http {
void http_snapshot::run() {
  timer_      = std::make_shared<timer_type>(g_io_context(), std::chrono::seconds(1));
  auto l_path = register_file_type::get_server_snapshot_path();

  if (!FSys::exists(l_path.parent_path())) {
    FSys::create_directories(l_path.parent_path());
  }

  default_logger_raw()->log(log_loc(), level::info, "http_snapshot laod {} ", l_path);
  snapshot::sqlite_snapshot l_snapshot{register_file_type::get_server_snapshot_path(), *g_reg()};
  l_snapshot.load<server_task_info>();
  do_wait();
}

void http_snapshot::do_wait() {
  if (app_base::GetPtr()->is_stop()) return;

  timer_->async_wait(boost::asio::bind_cancellation_slot(
      app_base::GetPtr()->on_cancel.slot(),
      [this](const boost::system::error_code& in_error_code) {
        if (in_error_code) {
          default_logger_raw()->log(log_loc(), level::err, "timer error: {}", in_error_code);
          return;
        }
        default_logger_raw()->log(log_loc(), level::info, "http_snapshot run");
        run_impl();
      }
  ));
}
void http_snapshot::run_impl() {}
}  // namespace doodle::http