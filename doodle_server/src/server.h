/*
 * @Author: your name
 * @Date: 2020-12-12 19:17:30
 * @LastEditTime: 2020-12-16 11:02:55
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_server\server.h
 */

#pragma once
#include <DoodleServer_global.h>
// #include <fileSystem.pb.h>
#include <fileSystem.grpc.pb.h>
#include <fileSystem.pb.h>
//后期希望可以去除
// #include <boost/filesystem.hpp>
// #include <doodle_test.pb.h>
DOODLE_NAMESPACE_S

class fileSystem final : public filesys::filesystem::Service {
 public:
  explicit fileSystem();
  ~fileSystem() override;
  grpc::Status exist(grpc::ServerContext* context, const filesys::path* request, filesys::path* response) override;
  grpc::Status createFolder(grpc::ServerContext* context, const filesys::path* request, filesys::path* response) override;
  grpc::Status rename(grpc::ServerContext* context, const filesys::path* request, filesys::path* response) override;
  grpc::Status open(grpc::ServerContext* context, grpc::ServerReaderWriter<filesys::io_stream, filesys::io_stream>* stream) override;
  grpc::Status copy(grpc::ServerContext* context, const filesys::copy_info* request, filesys::path* response) override;
};

// class fileSystem final : public filesys::filesystem::AsyncService {
//  public:
//   explicit fileSystem();
//   ~fileSystem() override;
// };
DOODLE_NAMESPACE_E