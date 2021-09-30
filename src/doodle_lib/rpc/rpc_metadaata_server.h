//
// Created by TD on 2021/5/25.
//

#pragma once
#include <doodle_lib/Exception/exception.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/libWarp/protobuf_warp.h>

#include <doodle_lib/libWarp/cache.hpp>
#include <doodle_lib/libWarp/lru_cache_policy.hpp>

namespace doodle {
class DOODLELIB_API rpc_metadaata_server final : public metadata_server::Service, public details::no_copy {
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

  grpc::Status install_metadata(grpc::ServerContext* context, const metadata_database* request, metadata_database* response) override;
  grpc::Status delete_metadata(grpc::ServerContext* context, const metadata_database* request, metadata_database* response) override;
  grpc::Status update_metadata(grpc::ServerContext* context, const metadata_database* request, metadata_database* response) override;
  grpc::Status filter_metadata(grpc::ServerContext* context, const metadata_database_filter* request, grpc::ServerWriter<metadata_database>* writer) override;
  grpc::Status install_user_date(::grpc::ServerContext* context, const ::doodle::user_database* request, ::doodle::user_database* response) override;
  grpc::Status update_user_date(::grpc::ServerContext* context, const ::doodle::user_database* request, ::doodle::user_database* response) override;
  grpc::Status delete_user_date(::grpc::ServerContext* context, const ::doodle::user_database_filter* request, ::doodle::user_database* response) override;
  grpc::Status filter_user_date(::grpc::ServerContext* context, const ::doodle::user_database_filter* request, ::grpc::ServerWriter<::doodle::user_database>* writer) override;
};

}  // namespace doodle
