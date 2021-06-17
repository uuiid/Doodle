//
// Created by TD on 2021/6/9.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/core/CoreSet.h>
#include <FileSystemServer.grpc.pb.h>

namespace doodle {

class DOODLELIB_API RpcFileSystemServer : public FileSystemServer::Service,public details::no_copy {
  CoreSet& p_set;

 public:
  explicit RpcFileSystemServer();

  virtual grpc::Status GetInfo(grpc::ServerContext* context,
                               const FileInfo* request,
                               FileInfo* response) override;

  virtual grpc::Status IsExist(grpc::ServerContext* context,
                               const FileInfo* request,
                               FileInfo* response) override;

  virtual grpc::Status IsFolder(grpc::ServerContext* context,
                                const FileInfo* request,
                                FileInfo* response);

  virtual grpc::Status GetSize(grpc::ServerContext* context,
                               const FileInfo* request,
                               FileInfo* response) override;

  virtual grpc::Status GetTimestamp(grpc::ServerContext* context,
                                    const FileInfo* request,
                                    FileInfo* response) override;

  virtual grpc::Status Download(grpc::ServerContext* context,
                                const FileInfo* request,
                                grpc::ServerWriter<FileStream>* writer) override;

  virtual grpc::Status Upload(grpc::ServerContext* context,
                              grpc::ServerReader<FileStream>* reader,
                              FileInfo* response) override;

};

}  // namespace doodle
