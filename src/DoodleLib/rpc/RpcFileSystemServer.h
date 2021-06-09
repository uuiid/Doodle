//
// Created by TD on 2021/6/9.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <FileSystemServer.grpc.pb.h>

namespace doodle {
class DOODLELIB_API RpcFileSystemServer : public FileSystemServer::Service {
 public:
  explicit RpcFileSystemServer();

  virtual grpc::Status GetInfo(grpc::ServerContext* context,
                               const fileInfo* request,
                               fileInfo* response) override;

  virtual grpc::Status IsExist(grpc::ServerContext* context,
                               const fileInfo* request,
                               fileInfo* response) override;

  virtual grpc::Status IsFolder(grpc::ServerContext* context,
                                const fileInfo* request,
                                fileInfo* response);
                                
  virtual grpc::Status GetSize(grpc::ServerContext* context,
                               const fileInfo* request,
                               fileInfo* response) override;


  virtual grpc::Status GetTimestamp(grpc::ServerContext* context,
                                    const fileInfo* request,
                                    fileInfo* response) override;


  virtual grpc::Status Download(grpc::ServerContext* context,
                                const fileInfo* request,
                                grpc::ServerWriter<fileStream>* writer) override;

  virtual grpc::Status Upload(grpc::ServerContext* context,
                              grpc::ServerReader<fileStream>* reader,
                              fileInfo* response) override;

  DOODLE_DISABLE_COPY(RpcFileSystemServer)
};

}  // namespace doodle
