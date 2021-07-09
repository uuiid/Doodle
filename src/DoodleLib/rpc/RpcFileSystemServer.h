//
// Created by TD on 2021/6/9.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/core/CoreSet.h>
#include <DoodleLib/libWarp/protobuf_warp.h>



namespace doodle {

class DOODLELIB_API RpcFileSystemServer : public FileSystemServer::Service,public details::no_copy {
  CoreSet& p_set;

 public:
  explicit RpcFileSystemServer();

  grpc::Status GetInfo(grpc::ServerContext* context,
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
                       grpc::ServerWriter< FileInfo > * writer) override;

  grpc::Status Download(grpc::ServerContext* context,
                                const FileInfo* request,
                                grpc::ServerWriter<FileStream>* writer) override;

  grpc::Status Upload(grpc::ServerContext* context,
                              grpc::ServerReader<FileStream>* reader,
                              FileInfo* response) override;

};

}  // namespace doodle
