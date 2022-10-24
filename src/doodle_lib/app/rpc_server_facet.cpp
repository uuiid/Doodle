//
// Created by TD on 2022/10/8.
//

#include "rpc_server_facet.h"
#include <doodle_lib/json_rpc/json_rpc_server.h>
#include <doodle_core/json_rpc/core/server.h>
#include <doodle_app/app/program_options.h>
#include <doodle_lib/long_task/image_to_move.h>
#include <boost/program_options.hpp>

#include <wil/result.h>
namespace doodle::facet {

class rpc_server_facet::impl {
 public:
  //  std::shared_ptr<json_rpc_server> rpc_server_attr;
  //  std::shared_ptr<json_rpc::server> server_attr;
  std::string name{"json_rpc"};

  boost::program_options::options_description opt{"rpc"};
  std::string files_attr;
  std::shared_ptr<program_options> program_options;

  void redirect_io_to_console() {
    boost::ignore_unused(this);
    /// 释放控制台
    CONSOLE_SCREEN_BUFFER_INFO conInfo;///控制台信息
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &conInfo);
    //    if (conInfo.dwSize.Y < minLength)
    //      conInfo.dwSize.Y = minLength;
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), conInfo.dwSize);

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

    // Clear the error state for each of the C++ standard streams.
    //    std::wcout.clear();
    //    std::cout.clear();
    //    std::wcerr.clear();
    //    std::cerr.clear();
    //    std::wcin.clear();
    //    std::cin.clear();
  }
};

rpc_server_facet::rpc_server_facet()
    : p_i(std::make_unique<impl>()) {
  //  p_i->redirect_io_to_console();
  //  p_i->rpc_server_attr = std::make_shared<json_rpc_server>();
}
const std::string& rpc_server_facet::name() const noexcept {
  return p_i->name;
}
void rpc_server_facet::operator()() {
  /// 开始创建视频
  if ((*p_i->program_options)["create_move"]) {
    if (!FSys::exists(p_i->files_attr)) {
      DOODLE_LOG_INFO("不存在文件 {}", p_i->files_attr);
    }
    FSys::ifstream l_file{p_i->files_attr};
    auto l_json     = nlohmann::json::parse(l_file);
    auto l_out_path = l_json["out_path"].get<FSys::path>();
    auto l_move     = l_json["image_attr"].get<std::vector<doodle::movie::image_attr>>();
    process_message l_msg;
    g_reg()->ctx().at<image_to_move>()->create_move(
        l_out_path,
        l_msg,
        l_move
    );
  }

  //  p_i->server_attr = std::make_shared<json_rpc::server>(g_io_context());
  //  p_i->server_attr->set_rpc_server(p_i->rpc_server_attr);
}
void rpc_server_facet::deconstruction() {
  //  p_i->server_attr->stop();
  //  p_i->server_attr.reset();
  //  p_i->rpc_server_attr.reset();
}
std::shared_ptr<json_rpc::server> rpc_server_facet::server_attr() const {
  //  return p_i->server_attr;
  return {};
}
void rpc_server_facet::add_program_options(const std::shared_ptr<program_options>& in_opt) {
  p_i->opt.add_options()("create_move", boost::program_options::value(&p_i->files_attr), "创建视频的序列json选项");
  in_opt->add_opt(p_i->opt);
  p_i->program_options = in_opt;
}

rpc_server_facet::~rpc_server_facet() = default;
}  // namespace doodle::facet
