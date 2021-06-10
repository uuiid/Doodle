#include "RpcServerHandle.h"

#include <DoodleLib/Logger/Logger.h>
#include <DoodleLib/rpc/RpcFileSystemServer.h>
#include <DoodleLib/rpc/RpcMetadaataServer.h>
#include <grpcpp/grpcpp.h>

namespace doodle {
RpcServerHandle::RpcServerHandle()
    : p_metadata_Server(),
      p_file_system_Server(),
      p_rpc_metadata_server(std::make_shared<RpcMetadaataServer>()),
      p_rpc_file_system_server(std::make_shared<RpcFileSystemServer>()),
      p_build(std::make_unique<grpc::ServerBuilder>()),
      p_thread() {
  grpc::ResourceQuota qu{"doodle_meta"};
  qu.SetMaxThreads(4);
  p_build->SetResourceQuota(qu);
}

void RpcServerHandle::runFileSystemServer(int port) {
  ///检查p_metadata_Server防止重复调用
  if (p_file_system_Server)
    return;
  std::string server_address{"[::]:"};
  server_address += std::to_string(port);

  p_build->AddListeningPort(server_address, grpc::InsecureServerCredentials());
  p_build->RegisterService(p_rpc_file_system_server.get());

  //  auto t = k_builder.BuildAndStart();
  p_file_system_Server = std::move(p_build->BuildAndStart());
  DOODLE_LOG_INFO("Server listening on " << server_address);
  p_thread = std::thread{[this]() {
    p_file_system_Server->Wait();
  }};
}

void RpcServerHandle::stopFileSystemServer() {
  if (!p_file_system_Server)
    return;
  using namespace std::chrono_literals;
  auto k_time = std::chrono::system_clock::now() + 2s;

  p_file_system_Server->Shutdown(k_time);
  if (p_thread.joinable())
    p_thread.join();

  p_file_system_Server.reset();
}

void RpcServerHandle::runMetadataServer(int port) {
  ///检查p_metadata_Server防止重复调用
  if (p_metadata_Server)
    return;
  std::string server_address{"[::]:"};
  server_address += std::to_string(port);

  p_build->AddListeningPort(server_address, grpc::InsecureServerCredentials());
  p_build->RegisterService(p_rpc_metadata_server.get());

  //  auto t = k_builder.BuildAndStart();
  p_metadata_Server = std::move(p_build->BuildAndStart());
  DOODLE_LOG_INFO("Server listening on " << server_address);
  p_thread = std::thread{[this]() {
    p_metadata_Server->Wait();
  }};
}

void RpcServerHandle::stopMetadataServer() {
  if (!p_metadata_Server)
    return;
  using namespace std::chrono_literals;
  auto k_time = std::chrono::system_clock::now() + 2s;

  p_metadata_Server->Shutdown(k_time);
  if (p_thread.joinable())
    p_thread.join();

  p_metadata_Server.reset();
}

void RpcServerHandle::runServer(int port_meta, int port_file_sys) {
  runMetadataServer(port_meta);
  runFileSystemServer(port_file_sys);
}

void RpcServerHandle::stop() {
  stopMetadataServer();
  stopFileSystemServer();
}
}  // namespace doodle