//
// Created by TD on 2023/12/1.
//
#include <doodle_core/database_task/sqlite_client.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/main_map.h>

#include <doodle_app/app/app_command.h>

#include <doodle_lib/doodle_lib_all.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/exe_warp/ue_exe.h>
int core_process_message(int argc, char* argv[]) {
  using main_app = doodle::app_command<>;
  main_app l_app{argc, argv};

  using namespace doodle;

  auto l_h   = entt::handle{*g_reg(), g_reg()->create()};

  auto l_log = l_h.emplace<process_message>("test").logger();

  l_log->log(log_loc(), level::info, "test");
  l_log->flush();
  std::this_thread::sleep_for(std::chrono::seconds(1));

  if (!l_h.get<process_message>().is_run()) return 1;

  l_log->log(log_loc(), level::off, "{}", magic_enum::enum_name(process_message::state::success));
  std::this_thread::sleep_for(std::chrono::seconds(1));

  if (!l_h.get<process_message>().is_success()) return 1;
  //  l_log->log(log_loc(), level::level_enum::n_levels, "{}", process_message::success);
  return 0;
}