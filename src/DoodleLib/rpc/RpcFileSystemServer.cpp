//
// Created by TD on 2021/6/9.
//

#include "RpcFileSystemServer.h"

#include <DoodleLib/core/CoreSet.h>
#include <google/protobuf/util/time_util.h>

#include <boost/format.hpp>

namespace doodle {
RpcFileSystemServer::RpcFileSystemServer()
    : FileSystemServer::Service(),
      p_set(CoreSet::getSet()) {
}

grpc::Status RpcFileSystemServer::GetInfo(grpc::ServerContext* context, const FileInfo* request, FileInfo* response) {
  FSys::path k_path = p_set.getDataRoot() / request->path();
  auto k_ex         = FSys::exists(k_path);
  if (!k_ex)
    return grpc::Status::OK;
  response->set_exist(k_ex);

  auto k_dir = FSys::is_directory(k_path);
  response->set_isfolder(k_dir);
  if (k_dir)
    return grpc::Status::OK;

  response->set_size(FSys::file_size(k_path));
  auto k_time        = FSys::last_write_time(k_path);
  auto k_google_time = google::protobuf::util::TimeUtil::TimeTToTimestamp(k_time);
  response->mutable_update_time()->CopyFrom(k_google_time);

  return grpc::Status::OK;
}

grpc::Status RpcFileSystemServer::IsExist(grpc::ServerContext* context, const FileInfo* request, FileInfo* response) {
  FSys::path k_path = p_set.getDataRoot() / request->path();
  auto k_ex         = FSys::exists(k_path);
  response->set_exist(k_ex);

  return grpc::Status::OK;
}

grpc::Status RpcFileSystemServer::IsFolder(grpc::ServerContext* context, const FileInfo* request, FileInfo* response) {
  FSys::path k_path = p_set.getDataRoot() / request->path();
  auto k_ex         = FSys::exists(k_path);
  response->set_exist(k_ex);
  if (!k_ex)
    return grpc::Status::OK;

  auto k_dir = FSys::is_directory(k_path);
  response->set_isfolder(k_dir);

  return grpc::Status::OK;
}

grpc::Status RpcFileSystemServer::GetSize(grpc::ServerContext* context, const FileInfo* request, FileInfo* response) {
  FSys::path k_path = p_set.getDataRoot() / request->path();
  auto k_ex         = FSys::exists(k_path);
  response->set_exist(k_ex);
  if (!k_ex)
    return grpc::Status::OK;

  auto k_dir = FSys::is_directory(k_path);
  response->set_isfolder(k_dir);
  if (k_dir)
    return grpc::Status::OK;

  response->set_size(FSys::file_size(k_path));

  return grpc::Status::OK;
}

grpc::Status RpcFileSystemServer::GetTimestamp(grpc::ServerContext* context, const FileInfo* request, FileInfo* response) {
  FSys::path k_path = p_set.getDataRoot() / request->path();
  auto k_ex         = FSys::exists(k_path);
  response->set_exist(k_ex);
  if (!k_ex)
    return grpc::Status::OK;

  auto k_dir = FSys::is_directory(k_path);
  response->set_isfolder(k_dir);
  if (k_dir)
    return grpc::Status::OK;

  response->set_size(FSys::file_size(k_path));
  auto k_time        = FSys::last_write_time(k_path);
  auto k_google_time = google::protobuf::util::TimeUtil::TimeTToTimestamp(k_time);
  response->mutable_update_time()->CopyFrom(k_google_time);

  return grpc::Status::OK;
}

grpc::Status RpcFileSystemServer::Download(grpc::ServerContext* context, const FileInfo* request, grpc::ServerWriter<FileStream>* writer) {
  FSys::path k_path = p_set.getDataRoot() / request->path();
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

  auto s_size = CoreSet::getBlockSize();
  FileStream k_stream{};

  while (k_file) {
    std::string k_value{};
    k_value.resize(s_size);
    k_file.read(k_value.data(), s_size);
    auto k_s = k_file.gcount();
    if (k_s != s_size)
      k_value.resize(k_s);

    k_stream.mutable_data()->set_value(std::move(k_value));
    if (!writer->Write(k_stream))
      return grpc::Status::CANCELLED;
  }

  return grpc::Status::OK;
}

grpc::Status RpcFileSystemServer::Upload(grpc::ServerContext* context, grpc::ServerReader<FileStream>* reader, FileInfo* response) {
  FileStream k_file_stream{};
  reader->Read(&k_file_stream);
  FSys::path k_path = p_set.getDataRoot() / k_file_stream.info().path();
  auto k_ex         = FSys::exists(k_path.parent_path());
  if (!k_ex)
    FSys::create_directories(k_path.parent_path());

  auto k_dir = FSys::is_directory(k_path);
  if (k_ex && k_dir)
    return grpc::Status::CANCELLED;

  FSys::ofstream k_file{k_path, std::ios::out | std::ios::binary};

  while (reader->Read(&k_file_stream)) {
    auto& str = k_file_stream.data().value();
    k_file.write(str.data(), str.size());
  }

  return grpc::Status::OK;
}
}  // namespace doodle
