#include "RpcServerHandle.h"

#include <DoodleLib/Logger/Logger.h>
#include <DoodleLib/rpc/RpcFileSystemServer.h>
#include <DoodleLib/rpc/RpcMetadaataServer.h>

namespace doodle {
RpcServerHandle::RpcServerHandle()
    : p_Server(),
      p_rpc_metadata_server(),
      p_rpc_file_system_server(),
      p_build(std::make_unique<grpc::ServerBuilder>()),
      p_thread() {
  grpc::ResourceQuota qu{"doodle_meta"};
  qu.SetMaxThreads(boost::numeric_cast<std::int32_t>(std::thread::hardware_concurrency()));
  p_build->SetResourceQuota(qu);
}

void RpcServerHandle::registerFileSystemServer(int port) {
  p_rpc_file_system_server = std::make_shared<RpcFileSystemServer>();
  std::string server_address{"[::]:"};
  server_address += std::to_string(port);

  p_build->AddListeningPort(server_address, grpc::InsecureServerCredentials());
  p_build->RegisterService(p_rpc_file_system_server.get());

  DOODLE_LOG_INFO(fmt::format("Server listening on {}", server_address));
}

void RpcServerHandle::registerMetadataServer(int port) {
  p_rpc_metadata_server = std::make_shared<RpcMetadaataServer>();
  std::string server_address{"[::]:"};
  server_address += std::to_string(port);

  p_build->AddListeningPort(server_address, grpc::InsecureServerCredentials());
  p_build->RegisterService(p_rpc_metadata_server.get());

  DOODLE_LOG_INFO(fmt::format("Server listening on {}", server_address));
}

void RpcServerHandle::runServer(int port_meta, int port_file_sys) {
  ///检查p_metadata_Server防止重复调用
  if (p_Server)
    return;

  registerMetadataServer(port_meta);
  registerFileSystemServer(port_file_sys);

  p_Server = std::move(p_build->BuildAndStart());
  if (!p_Server)
    throw DoodleError{"无法创建服务器"};

  p_thread = std::thread{[this]() {
    p_Server->Wait();
  }};
}
void RpcServerHandle::runServerWait(int port_meta, int port_file_sys) {
  runServer(port_meta, port_file_sys);
  p_thread.join();
}
void RpcServerHandle::stop() {
  using namespace chrono::literals;
  auto k_time = chrono::system_clock::now() + 2s;

  p_Server->Shutdown(k_time);
  if (p_thread.joinable())
    p_thread.join();

  p_Server.reset();
}
RpcServerHandle::~RpcServerHandle() {
  stop();
}

}  // namespace doodle
