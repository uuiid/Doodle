//
// Created by TD on 2021/5/25.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Exception/exception.h>
#include <DoodleLib/core/core_set.h>
#include <DoodleLib/libWarp/protobuf_warp.h>

#include <DoodleLib/libWarp/cache.hpp>
#include <DoodleLib/libWarp/lru_cache_policy.hpp>

namespace doodle {
class DOODLELIB_API rpc_metadaata_server final : public MetadataServer::Service, public details::no_copy {
  core_set& p_set;

  std::thread p_thread;

  caches::fixed_sized_cache<std::string, std::string, caches::LRUCachePolicy<std::string>> p_cache;

  [[nodiscard]] inline FSys::path getPath(const std::string& in_string) const {
    if (in_string.empty()) {
      throw doodle_error{"str 是空的"};
    }
    return p_set.get_cache_root() / in_string;
  };

  [[nodiscard]] inline FSys::path get_delete_path(const std::string& in_string) const {
    if (in_string.empty())
      throw doodle_error{"str 是空的"};
    return p_set.get_cache_root("delete") / in_string;
  };

  [[nodiscard]] std::string get_cache_and_file(const FSys::path& key);
  void put_cache_and_file(const FSys::path& key, const std::string& value);

  //  [[nodiscard]] FSys::path getPath(uint64_t id,const std::string& in_string)const;
 public:
  rpc_metadaata_server();

  grpc::Status InstallMetadata(grpc::ServerContext* context, const DataDb* request, DataDb* response) override;
  grpc::Status DeleteMetadata(grpc::ServerContext* context, const DataDb* request, DataDb* response) override;
  grpc::Status UpdateMetadata(grpc::ServerContext* context, const DataDb* request, DataDb* response) override;
  grpc::Status FilterMetadata(grpc::ServerContext* context, const DataDb_Filter* request, grpc::ServerWriter<DataDb>* writer) override;
  virtual grpc::Status InstallUserDate(::grpc::ServerContext* context, const ::doodle::user_database* request, ::doodle::user_database* response) override;
  virtual grpc::Status UpdateUserDate(::grpc::ServerContext* context, const ::doodle::user_database* request, ::doodle::user_database* response) override;
  virtual grpc::Status DeleteUserDate(::grpc::ServerContext* context, const ::doodle::user_database_filter* request, ::doodle::user_database* response) override;
  virtual grpc::Status FilterUserDate(::grpc::ServerContext* context, const ::doodle::user_database_filter* request, ::grpc::ServerWriter<::doodle::user_database>* writer) override;
};

}  // namespace doodle
