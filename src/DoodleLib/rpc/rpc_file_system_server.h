//
// Created by TD on 2021/6/9.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/core/core_set.h>
#include <DoodleLib/libWarp/protobuf_warp.h>

#include <DoodleLib/libWarp/cache.hpp>
#include <DoodleLib/libWarp/lru_cache_policy.hpp>
namespace doodle {
namespace rpc_filesystem {
class DOODLELIB_API file_hash : public details::no_copy {
  using time_pos = chrono::sys_time_pos;
  FSys::path _path;
  std::size_t _size;
  time_pos _time;
  std::string _hash;
  mutable std::mutex _mutex;

 public:
  explicit file_hash(FSys::path path);

  bool valid() const;
  [[nodiscard]] std::string hash() const;
  void update_hash();
};
using file_hash_ptr = std::shared_ptr<file_hash>;

class DOODLELIB_API file_mutex {
  std::recursive_mutex _mutex;

 public:
  file_mutex();

  decltype(_mutex)& mutex();
  // explicit ifstream(FSys::path path,
  //                   std::ios_base::openmode _Mode = ios_base::in,
  //                   int _Prot                     = std::ios_base::_Default_open_prot);
};
using file_mutex_ptr = std::shared_ptr<file_mutex>;
}  // namespace rpc_filesystem

class DOODLELIB_API rpc_file_system_server
    : public file_system_server::Service,
      public details::no_copy {
  core_set& p_set;
  caches::fixed_sized_cache<std::string, rpc_filesystem::file_hash_ptr, caches::LRUCachePolicy<std::string>> p_cache;
  caches::fixed_sized_cache<std::string, rpc_filesystem::file_mutex_ptr, caches::LRUCachePolicy<std::string>> _mutex;

  rpc_filesystem::file_mutex_ptr get_mutex(const FSys::path& in_path);

 public:
  explicit rpc_file_system_server();

  grpc::Status get_info(grpc::ServerContext* context,
                       const file_info_server* request,
                       file_info_server* response) override;

  grpc::Status get_hash(grpc::ServerContext* context,
                       const file_info_server* request,
                       file_info_server* response) override;

  grpc::Status is_exist(grpc::ServerContext* context,
                       const file_info_server* request,
                       file_info_server* response) override;

  grpc::Status is_folder(grpc::ServerContext* context,
                        const file_info_server* request,
                        file_info_server* response) override;

  grpc::Status get_size(grpc::ServerContext* context,
                       const file_info_server* request,
                       file_info_server* response) override;

  grpc::Status get_timestamp(grpc::ServerContext* context,
                            const file_info_server* request,
                            file_info_server* response) override;

  grpc::Status get_list(grpc::ServerContext* context,
                       const file_info_server* request,
                       grpc::ServerWriter<file_info_server>* writer) override;

  grpc::Status download(grpc::ServerContext* context,
                        const file_info_server* request,
                        grpc::ServerWriter<file_stream_server>* writer) override;

  grpc::Status upload(grpc::ServerContext* context,
                      grpc::ServerReader<file_stream_server>* reader,
                      file_info_server* response) override;

  grpc::Status move(grpc::ServerContext* context,
                    const file_info_move_server* request,
                    file_info_server* response) override;
};

}  // namespace doodle
