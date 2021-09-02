#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <grpcpp/grpcpp.h>

namespace doodle {
class DOODLELIB_API RpcServerHandle {
  std::unique_ptr<grpc::Server> p_Server;

  RpcMetadataServerPtr p_rpc_metadata_server;
  RpcFileSystemServerPtr p_rpc_file_system_server;
  std::unique_ptr<grpc::ServerBuilder> p_build;

  std::thread p_thread;

 public:
  RpcServerHandle();
  ~RpcServerHandle();
  void registerFileSystemServer(int port);
  void registerMetadataServer(int port);

  void runServer(int port_meta, int port_file_sys);
  void runServerWait(int port_meta, int port_file_sys);
  void stop();
};
}  // namespace doodle
