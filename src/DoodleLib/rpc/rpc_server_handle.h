#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <grpcpp/grpcpp.h>

namespace doodle {
class DOODLELIB_API rpc_server_handle {
  std::unique_ptr<grpc::Server> p_Server;

  RpcMetadataServerPtr p_rpc_metadata_server;
  RpcFileSystemServerPtr p_rpc_file_system_server;
  std::unique_ptr<grpc::ServerBuilder> p_build;

  std::thread p_thread;

 public:
  rpc_server_handle();
  ~rpc_server_handle();
  void register_file_system_server(int port);
  void register_metadata_server(int port);

  void run_server(int port_meta, int port_file_sys);
  void run_server_wait(int port_meta, int port_file_sys);
  void stop();
};
}  // namespace doodle
