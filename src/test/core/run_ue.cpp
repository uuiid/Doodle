//
// Created by td_main on 2023/9/6.
//

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio.hpp>
#include <boost/process/v2/process.hpp>
#include <boost/process/v2/stdio.hpp>
int core_run_ue(int, char** const) {
  boost::asio::io_context l_io_context;
  static FSys::path l_ue_path{R"(D:\Program Files\Epic Games\UE_5.2\Engine\Binaries\Win64\UE4Editor.exe)"};
  static std::string l_command_line{R"(--version)"};
  boost::asio::readable_pipe log_{l_io_context};
  boost::asio::readable_pipe err_{l_io_context};
  boost::process::v2::process l_child{
      g_io_context(), ue_path_.string(), l_command_line, boost::process::v2::process_stdio{log_, err_, {}}};
}