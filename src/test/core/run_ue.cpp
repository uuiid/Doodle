//
// Created by td_main on 2023/9/6.
//

#include "doodle_core/core/app_base.h"

#include "doodle_lib/exe_warp/ue_exe.h"
#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio.hpp>
using namespace doodle;

int core_run_ue(int, char** const) {
  app_base l_base{};
  boost::asio::co_spawn(g_io_context(),
                        async_run_ue("--version", spdlog::default_logger()), boost::asio::detached);
  return 0;
}