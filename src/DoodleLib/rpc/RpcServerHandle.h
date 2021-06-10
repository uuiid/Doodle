#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <grpcpp/grpcpp.h>

namespace doodle {
class RpcServerHandle {
  std::unique_ptr<grpc::Server> p_metadata_Server;
  std::unique_ptr<grpc::Server> p_file_system_Server;

  RpcMetadataServerPtr p_rpc_metadata_server;
  RpcFileSystemServerPtr p_rpc_file_system_server;
  std::unique_ptr<grpc::ServerBuilder> p_build;

  std::thread p_thread;

 public:
  RpcServerHandle();

  void runFileSystemServer(int port);
  void stopFileSystemServer();
  void runMetadataServer(int port);
  void stopMetadataServer();

  void runServer(int port_meta, int port_file_sys);
  void stop();
};
}  // namespace doodle