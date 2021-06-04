//
// Created by TD on 2021/5/25.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/core/CoreSet.h>

#include <DoodleLib/libWarp/cache.hpp>
#include <DoodleLib/libWarp/lru_cache_policy.hpp>
#include <MetadataServer.grpc.pb.h.>

namespace doodle {
class DOODLELIB_API RpcServer final : public MetadataServer::Service {
  CoreSet& p_set;
  std::unique_ptr<grpc::Server> p_Server;

  std::thread p_thread;

  caches::fixed_sized_cache<std::string, std::string, caches::LRUCachePolicy<std::string>> p_cache;

  [[nodiscard]] inline FSys::path getPath(const std::string& in_string) const {
    if (in_string.empty())
      throw DoodleError{"str 是空的"};
    return p_set.getCacheRoot() / in_string;
  };

  [[nodiscard]] std::string get_cache_and_file(const FSys::path& key);
  [[nodiscard]] void put_cache_and_file(const FSys::path& key, const std::string& value);

  //  [[nodiscard]] FSys::path getPath(uint64_t id,const std::string& in_string)const;
 public:
  RpcServer();
  grpc::Status GetProject(grpc::ServerContext* context, const google::protobuf::Empty* request, DataVector* response) override;
  grpc::Status GetChild(grpc::ServerContext* context, const DataDb* request, DataVector* response) override;
  grpc::Status GetMetadata(grpc::ServerContext* context, const DataDb* request, DataDb* response) override;
  grpc::Status InstallMetadata(grpc::ServerContext* context, const DataDb* request, DataDb* response) override;
  grpc::Status DeleteMetadata(grpc::ServerContext* context, const DataDb* request, DataDb* response) override;
  grpc::Status UpdataMetadata(grpc::ServerContext* context, const DataDb* request, DataDb* response) override;

  DOODLE_DISABLE_COPY(RpcServer)
};

class RpcServerHandle {
  std::unique_ptr<grpc::Server> p_Server;

  RpcServerPtr p_rpc_server;
  std::unique_ptr<grpc::ServerBuilder> p_build;

  std::thread p_thread;

 public:
  RpcServerHandle();

  void runServer(int port);
  void stop();
};
}  // namespace doodle
