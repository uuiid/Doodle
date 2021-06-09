//
// Created by TD on 2021/6/9.
//

#include "RpcFileSystemServer.h"

#include <google/protobuf/util/time_util.h>

#include <boost/format.hpp>

namespace doodle {
RpcFileSystemServer::RpcFileSystemServer() {
}

grpc::Status RpcFileSystemServer::GetInfo(grpc::ServerContext* context, const FileInfo* request, FileInfo* response) {
  FSys::path k_path = request->path();
  auto k_ex         = FSys::exists(k_path);
  response->set_exist(k_ex);
  if (!k_ex)
    return grpc::Status::OK;

  auto k_dir = FSys::is_directory(k_path);
  if (k_dir)
    return grpc::Status::OK;
  response->set_isfolder(k_dir);

  response->set_size(FSys::file_size(k_path));
  auto k_time        = FSys::last_write_time(k_path);
  auto k_google_time = google::protobuf::util::TimeUtil::TimeTToTimestamp(k_time);
  response->mutable_update_time()->CopyFrom(k_google_time);

  return grpc::Status::OK;
}

grpc::Status RpcFileSystemServer::IsExist(grpc::ServerContext* context, const FileInfo* request, FileInfo* response) {
  FSys::path k_path = request->path();
  auto k_ex         = FSys::exists(k_path);
  response->set_exist(k_ex);

  return grpc::Status::OK;
}

grpc::Status RpcFileSystemServer::IsFolder(grpc::ServerContext* context, const FileInfo* request, FileInfo* response) {
  FSys::path k_path = request->path();
  auto k_ex         = FSys::exists(k_path);
  response->set_exist(k_ex);
  if (!k_ex)
    return grpc::Status::OK;

  auto k_dir = FSys::is_directory(k_path);
  if (k_dir)
    return grpc::Status::OK;
  response->set_isfolder(k_dir);

  return grpc::Status::OK;
}

grpc::Status RpcFileSystemServer::GetSize(grpc::ServerContext* context, const FileInfo* request, FileInfo* response) {
  FSys::path k_path = request->path();
  auto k_ex         = FSys::exists(k_path);
  response->set_exist(k_ex);
  if (!k_ex)
    return grpc::Status::OK;

  auto k_dir = FSys::is_directory(k_path);
  if (k_dir)
    return grpc::Status::OK;
  response->set_isfolder(k_dir);

  response->set_size(FSys::file_size(k_path));

  return grpc::Status::OK;
}

grpc::Status RpcFileSystemServer::GetTimestamp(grpc::ServerContext* context, const FileInfo* request, FileInfo* response) {
  FSys::path k_path = request->path();
  auto k_ex         = FSys::exists(k_path);
  response->set_exist(k_ex);
  if (!k_ex)
    return grpc::Status::OK;

  auto k_dir = FSys::is_directory(k_path);
  if (k_dir)
    return grpc::Status::OK;
  response->set_isfolder(k_dir);

  response->set_size(FSys::file_size(k_path));
  auto k_time        = FSys::last_write_time(k_path);
  auto k_google_time = google::protobuf::util::TimeUtil::TimeTToTimestamp(k_time);
  response->mutable_update_time()->CopyFrom(k_google_time);

  return grpc::Status::OK;
}

grpc::Status RpcFileSystemServer::Download(grpc::ServerContext* context, const FileInfo* request, grpc::ServerWriter<FileStream>* writer) {
  FSys::path k_path = request->path();
  auto k_ex         = FSys::exists(k_path);
  auto k_dir        = FSys::is_directory(k_path);
  if (!k_ex || k_dir)
    return grpc::Status::CANCELLED;
  FSys::ifstream k_file{k_path, std::ios::in | std::ios::binary};

  if (!k_file.is_open() || !k_file.good()) {
    boost::format error{"eofbit: %b; failbit: %b; badbit: %b;"};
    error % k_file.eof() % k_file.fail() % k_file.bad();
    return {grpc::StatusCode::RESOURCE_EXHAUSTED, error.str()};
  }
  std::istreambuf_iterator<char> k_iter{k_file};

  std::string value{};

  FileStream k_stream{};
  for (; k_iter != std::istreambuf_iterator{}; ++k_iter) {
    value.push_back(*k_iter);
    if (value.size() == 3 * 1024 * 1024) {
      writer->Write()
    }
  }

  return grpc::Status::OK;
}

grpc::Status RpcFileSystemServer::Upload(grpc::ServerContext* context, grpc::ServerReader<FileStream>* reader, FileInfo* response) {
  return grpc::Status::OK;
}
}  // namespace doodle