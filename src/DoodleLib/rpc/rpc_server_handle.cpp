#include "rpc_server_handle.h"

#include <DoodleLib/Logger/logger.h>
#include <DoodleLib/rpc/rpc_file_system_server.h>
#include <DoodleLib/rpc/rpc_metadaata_server.h>

#include <csignal>

namespace doodle {
rpc_server_handle::rpc_server_handle()
    : p_Server(),
      p_rpc_metadata_server(),
      p_rpc_file_system_server(),
      p_build(std::make_unique<grpc::ServerBuilder>()) {
  grpc::ResourceQuota qu{"doodle_meta"};
  qu.SetMaxThreads(boost::numeric_cast<std::int32_t>(std::thread::hardware_concurrency()));
  p_build->SetResourceQuota(qu);
  DOODLE_LOG_INFO("开始创建rpc服务器");
}

void rpc_server_handle::register_file_system_server(int port) {
  p_rpc_file_system_server = new_object<rpc_file_system_server>();
  std::string server_address{"[::]:"};
  server_address += std::to_string(port);

  p_build->AddListeningPort(server_address, grpc::InsecureServerCredentials());
  p_build->RegisterService(p_rpc_file_system_server.get());

  DOODLE_LOG_INFO(fmt::format("Server listening on {}", server_address));
}

void rpc_server_handle::register_metadata_server(int port) {
  p_rpc_metadata_server = new_object<rpc_metadaata_server>();
  std::string server_address{"[::]:"};
  server_address += std::to_string(port);

  p_build->AddListeningPort(server_address, grpc::InsecureServerCredentials());
  p_build->RegisterService(p_rpc_metadata_server.get());

  DOODLE_LOG_INFO(fmt::format("Server listening on {}", server_address));
}

void rpc_server_handle::run_server(int port_meta, int port_file_sys) {
  ///检查p_metadata_Server防止重复调用
  if (p_Server)
    return;

  register_metadata_server(port_meta);
  register_file_system_server(port_file_sys);

  p_Server = std::move(p_build->BuildAndStart());
  if (!p_Server)
    throw doodle_error{"无法创建服务器"};
}

#if defined( _WIN32) and defined( _MSC_VER )

//BOOL WINAPI CtrlHandler(DWORD fdwCtrlType) {
//  DOODLE_LOG_WARN("收到退出信号， 开始退出 {}", fdwCtrlType);
//  CoreSet::getSet().p_stop = true;
//  CoreSet::getSet().p_condition.notify_all();
//  return true;
//}

#endif


void rpc_server_handle::run_server_wait(int port_meta, int port_file_sys) {
  run_server(port_meta, port_file_sys);
  auto k_ = [](int) {
    DOODLE_LOG_WARN("std  收到退出信号， 开始退出 ");
    core_set::getSet().p_stop = true;
    core_set::getSet().p_condition.notify_all();
  };
  std::signal(SIGABRT, k_);
#if defined( _WIN32) and defined( _MSC_VER )
  std::signal(SIGABRT_COMPAT, k_);
  std::signal(SIGBREAK, k_);
#endif
  std::signal(SIGFPE, k_);
  std::signal(SIGILL, k_);
  std::signal(SIGINT, k_);
  std::signal(SIGSEGV, k_);
  std::signal(SIGTERM, k_);

  auto& set = core_set::getSet();
  std::unique_lock k_lock{set.p_mutex};
  set.p_condition.wait(
      k_lock,
      [&set](){return set.p_stop;}
      );
}
void rpc_server_handle::stop() {
  using namespace chrono::literals;
  auto k_time = chrono::system_clock::now() + 2s;

  p_Server->Shutdown(k_time);

  p_Server.reset();
}
rpc_server_handle::~rpc_server_handle() {
  stop();
}

}  // namespace doodle
