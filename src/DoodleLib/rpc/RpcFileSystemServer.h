//
// Created by TD on 2021/6/9.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/core/CoreSet.h>
#include <DoodleLib/libWarp/protobuf_warp.h>

#include <DoodleLib/libWarp/cache.hpp>
#include <DoodleLib/libWarp/lru_cache_policy.hpp>
namespace doodle {
namespace rpc_filesystem {
class DOODLELIB_API file_hash : public details::no_copy {
  using time_pos = std::chrono::time_point<std::chrono::system_clock>;
  FSys::path _path;
  std::size_t _size;
  time_pos _time;
  std::string _hash;
  mutable std::mutex _mutex;

 public:
  explicit file_hash(FSys::path path);

  bool valid() const;
  [[nodiscard]] std::string hash() const;
  void undate_hash();
};
using file_hash_ptr = std::shared_ptr<file_hash>;

class DOODLELIB_API file_io : public details::no_copy {
 public:
  explicit file_io(FSys::path path);
};
}  // namespace rpc_filesystem

class DOODLELIB_API RpcFileSystemServer
    : public FileSystemServer::Service,
      public details::no_copy {
  CoreSet& p_set;
  caches::fixed_sized_cache<std::string, rpc_filesystem::file_hash_ptr, caches::LRUCachePolicy<std::string>> p_cache;


 public:
  explicit RpcFileSystemServer();

  grpc::Status GetInfo(grpc::ServerContext* context,
                       const FileInfo* request,
                       FileInfo* response) override;

  virtual grpc::Status GetHash(grpc::ServerContext* context,
                               const FileInfo* request,
                               FileInfo* response) override;

  grpc::Status IsExist(grpc::ServerContext* context,
                       const FileInfo* request,
                       FileInfo* response) override;

  grpc::Status IsFolder(grpc::ServerContext* context,
                        const FileInfo* request,
                        FileInfo* response);

  grpc::Status GetSize(grpc::ServerContext* context,
                       const FileInfo* request,
                       FileInfo* response) override;

  grpc::Status GetTimestamp(grpc::ServerContext* context,
                            const FileInfo* request,
                            FileInfo* response) override;

  grpc::Status GetList(grpc::ServerContext* context,
                       const FileInfo* request,
                       grpc::ServerWriter<FileInfo>* writer) override;

  grpc::Status Download(grpc::ServerContext* context,
                        const FileInfo* request,
                        grpc::ServerWriter<FileStream>* writer) override;

  grpc::Status Upload(grpc::ServerContext* context,
                      grpc::ServerReader<FileStream>* reader,
                      FileInfo* response) override;

  grpc::Status Move(grpc::ServerContext* context,
                    const FileInfoMove* request,
                    FileInfo* response) override;
};

}  // namespace doodle
