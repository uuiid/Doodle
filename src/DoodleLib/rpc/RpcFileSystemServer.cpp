//
// Created by TD on 2021/6/9.
//

#include "RpcFileSystemServer.h"

#include <DoodleLib/core/CoreSet.h>
#include <Logger/Logger.h>
#include <google/protobuf/util/time_util.h>

#include <boost/format.hpp>

namespace doodle {
RpcFileSystemServer::RpcFileSystemServer()
    : FileSystemServer::Service(),
      p_set(CoreSet::getSet()) {
}

grpc::Status RpcFileSystemServer::GetInfo(grpc::ServerContext* context, const FileInfo* request, FileInfo* response) {
  FSys::path k_path = p_set.getDataRoot() / request->path();
  DOODLE_LOG_DEBUG(fmt::format("get info path: {}", k_path));

  auto k_ex = FSys::exists(k_path);
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
  DOODLE_LOG_DEBUG(fmt::format("get exist path: {}", k_path));
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
  DOODLE_LOG_DEBUG(("get is dir path: {}", k_path));
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

  DOODLE_LOG_DEBUG(fmt::format("get size path: {}", k_path));

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

  DOODLE_LOG_DEBUG(fmt::format("get time path: {}", k_path));

  return grpc::Status::OK;
}

grpc::Status RpcFileSystemServer::GetList(grpc::ServerContext* context, const FileInfo* request, grpc::ServerWriter<FileInfo>* writer) {
  auto k_root       = p_set.getDataRoot();
  FSys::path k_path = p_set.getDataRoot() / request->path();

  DOODLE_LOG_DEBUG(fmt::format("list info path: {}", k_path));

  auto k_ex  = FSys::exists(k_path);
  auto k_dir = FSys::is_directory(k_path);
  if (!k_ex || !k_dir) {
    boost::format str{"路径: %s 存在: %b 是目录: %b"};
    str % k_path % k_ex % k_dir;
    DOODLE_LOG_DEBUG(fmt::format("路径: {} 存在: {} 是目录: {}", k_path, k_ex, k_dir))
    return {
        grpc::StatusCode::CANCELLED, fmt::format("路径: {} 存在: {} 是目录: {}", k_path, k_ex, k_dir)};
  }

  FileInfo k_info{};
  for (const auto& k_it : FSys::directory_iterator{k_path}) {
    k_info.set_path(std::move(k_it.path().lexically_relative(k_root).generic_string()));
    k_info.set_exist(true);
    k_info.set_isfolder(FSys::is_directory(k_it));
    if (!writer->Write(k_info))
      return grpc::Status::CANCELLED;
  }
  return grpc::Status::OK;
}

grpc::Status RpcFileSystemServer::Download(grpc::ServerContext* context, const FileInfo* request, grpc::ServerWriter<FileStream>* writer) {
  FSys::path k_path = p_set.getDataRoot() / request->path();
  auto k_ex         = FSys::exists(k_path);
  auto k_dir        = FSys::is_directory(k_path);
  if (!k_ex || k_dir)
    return grpc::Status::CANCELLED;

  DOODLE_LOG_DEBUG(fmt::format("down path: {}", k_path));

  {
    FSys::ifstream k_file{k_path, std::ios::in | std::ios::binary};

    if (!k_file.is_open() || !k_file.good()) {
      DOODLE_LOG_ERROR(fmt::format(
          "eofbit: %b; failbit: %b; badbit: %b;", k_file.eof(),
          k_file.fail(),
          k_file.bad()))
      return {grpc::StatusCode::RESOURCE_EXHAUSTED,
              fmt::format(
                  "eofbit: %b; failbit: %b; badbit: %b;", k_file.eof(),
                  k_file.fail(),
                  k_file.bad())};
    }
    //  std::istreambuf_iterator<char> k_iter{k_file};

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
  }

  return grpc::Status::OK;
}

grpc::Status RpcFileSystemServer::Upload(grpc::ServerContext* context, grpc::ServerReader<FileStream>* reader, FileInfo* response) {
  FileStream k_file_stream{};
  reader->Read(&k_file_stream);
  FSys::path k_path = p_set.getDataRoot() / k_file_stream.info().path();
  auto k_ex         = FSys::exists(k_path.parent_path());

  DOODLE_LOG_DEBUG(fmt::format("upload path: {}", k_path));

  if (!k_ex)
    FSys::create_directories(k_path.parent_path());

  auto k_dir = FSys::is_directory(k_path);
  if (k_dir)
    return {grpc::StatusCode::CANCELLED, k_path.generic_string() + " is dir"};

  {
    FSys::ofstream k_file{k_path, std::ios::out | std::ios::binary};

    if (!k_file.is_open() || !k_file.good()) {
      DOODLE_LOG_ERROR(fmt::format("eofbit: %b; failbit: %b; badbit: %b;", k_file.eof(),
                                   k_file.fail(),
                                   k_file.bad()))
      return {grpc::StatusCode::RESOURCE_EXHAUSTED,
              fmt::format("eofbit: %b; failbit: %b; badbit: %b;", k_file.eof(),
                          k_file.fail(),
                          k_file.bad())};
    }

    while (reader->Read(&k_file_stream)) {
      auto& str = k_file_stream.data().value();
      k_file.write(str.data(), str.size());
    }
  }

  return grpc::Status::OK;
}

}  // namespace doodle
