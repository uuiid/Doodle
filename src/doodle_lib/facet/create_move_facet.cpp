//
// Created by TD on 2023/2/2.
//

#include "create_move_facet.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/logger/logger.h>

#include <doodle_app/app/program_options.h>

#include "doodle_lib/distributed_computing/server.h"
#include "doodle_lib/long_task/image_to_move.h"

#include <boost/program_options.hpp>

#include <spdlog/sinks/stdout_sinks.h>
// #include <wil/result.h>
namespace doodle::facet {

namespace {
void open_console() {
  /// 释放控制台
  CONSOLE_SCREEN_BUFFER_INFO con_info;  /// 控制台信息
  GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &con_info);
  //    if (conInfo.dwSize.Y < minLength)
  //      conInfo.dwSize.Y = minLength;
  SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), con_info.dwSize);

  if (::AttachConsole(ATTACH_PARENT_PROCESS))
    DOODLE_LOG_INFO("附加到控制台");
  else
    ::AllocConsole();

  FILE* fp;
  // Redirect STDIN if the console has an input handle
  if (GetStdHandle(STD_INPUT_HANDLE) != INVALID_HANDLE_VALUE)
    if (freopen_s(&fp, "CONIN$", "r", stdin) != 0)
      DOODLE_LOG_INFO("打开std 句柄错误");
    else
      setvbuf(stdin, NULL, _IONBF, 0);

  // Redirect STDOUT if the console has an output handle
  if (GetStdHandle(STD_OUTPUT_HANDLE) != INVALID_HANDLE_VALUE)
    if (freopen_s(&fp, "CONOUT$", "w", stdout) != 0)
      DOODLE_LOG_INFO("打开std 句柄错误");
    else
      setvbuf(stdout, NULL, _IONBF, 0);

  // Redirect STDERR if the console has an error handle
  if (GetStdHandle(STD_ERROR_HANDLE) != INVALID_HANDLE_VALUE)
    if (freopen_s(&fp, "CONOUT$", "w", stderr) != 0)
      DOODLE_LOG_INFO("打开std 句柄错误");
    else
      setvbuf(stderr, NULL, _IONBF, 0);

  // Make C++ standard streams point to console as well.
  std::ios::sync_with_stdio(true);
}
}  // namespace

const std::string& create_move_facet::name() const noexcept { return name_; }
void create_move_facet::operator()() {
  //  open_console();
  //  logger_ctrl::get_log().add_log_sink(std::make_shared<spdlog::sinks::stdout_sink_mt>());
  if (!g_reg()->ctx().contains<image_to_move>())
    g_reg()->ctx().emplace<image_to_move>(std::make_shared<detail::image_to_move>());

  if (!FSys::exists(files_attr)) {
    DOODLE_LOG_INFO("不存在文件 {}", files_attr);
  }
  FSys::ifstream l_file{files_attr};
  entt::handle l_handle{make_handle()};
  auto l_json = nlohmann::json::parse(l_file);
  l_handle.emplace<FSys::path>(l_json["out_path"].get<FSys::path>());
  g_reg()->ctx().at<image_to_move>()->async_create_move(
      l_handle, l_json["image_attr"].get<std::vector<doodle::movie::image_attr>>(),
      [l_w = boost::asio::make_work_guard(g_io_context())]() { app_base::Get().stop_app(); }
  );
}
void create_move_facet::deconstruction() {}
void create_move_facet::add_program_options() {
  opt.add_options()("config_path", boost::program_options::value(&files_attr), "创建视频的序列json选项");
  auto& l_p = program_options::value();
  l_p.add_opt(opt);
}
}  // namespace doodle::facet