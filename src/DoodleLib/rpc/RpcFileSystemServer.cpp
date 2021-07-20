//
// Created by TD on 2021/6/9.
//

#include "RpcFileSystemServer.h"

#include <DoodleLib/core/CoreSet.h>
#include <Logger/Logger.h>
#include <libWarp/protobuf_warp_cpp.h>

namespace doodle {
rpc_filesystem::file_mutex_ptr RpcFileSystemServer::get_mutex(const FSys::path& in_path) {
  auto k_str = in_path.generic_string();

  if (!_mutex.Cached(k_str)) {
    _mutex.Put(k_str, std::make_shared<rpc_filesystem::file_mutex>());
  }
  return _mutex.Get(k_str);
}

RpcFileSystemServer::RpcFileSystemServer()
    : FileSystemServer::Service(),
      p_set(CoreSet::getSet()),
      p_cache(1024 * 1024 * 10),
      _mutex(1024 * 1024 * 100) {
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
  auto k_time        = FSys::last_write_time_t(k_path);
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
  auto k_time        = FSys::last_write_time_t(k_path);
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
    std::lock_guard k_lock{get_mutex(k_path)->mutex()};
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
    return {grpc::StatusCode::CANCELLED, fmt::format("{} is dir", k_path.generic_string())};

  {
    std::lock_guard k_lock{get_mutex(k_path)->mutex()};
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

grpc::Status RpcFileSystemServer::Move(grpc::ServerContext* context,
                                       const FileInfoMove* request,
                                       FileInfo* response) {
  if (!request->has_source()) {
    DOODLE_LOG_WARN("传入参数无效, 必须来源路径")
    return {grpc::StatusCode::CANCELLED, "传入参数无效, 必须来源路径"};
  }

  FSys::path k_s = p_set.getDataRoot() / request->source().path();

  if (!FSys::exists(k_s)) {
    DOODLE_LOG_WARN("来源路径不存在 {}", k_s)
    return {grpc::StatusCode::CANCELLED, "来源路径不存在"};
  }

  FSys::path k_t{};

  if (request->has_target())
    k_t = p_set.getDataRoot() / request->target().path();
  else
    k_t = p_set.getCacheRoot("delete") / k_s.lexically_proximate(p_set.getCacheRoot());

  if (!FSys::exists(k_t.parent_path()))
    FSys::create_directories(k_t.parent_path());

  try {
    ///如果目标存在则直接添加时间戳重命名
    if (FSys::exists(k_t)) {
      k_t = FSys::add_time_stamp(k_t);
    }

    std::lock_guard k_lock{get_mutex(k_s)->mutex()};
    FSys::rename(k_s, k_t);
    DOODLE_LOG_INFO("{} -move-> {}", k_s, k_t);
  } catch (const FSys::filesystem_error& e) {
    DOODLE_LOG_WARN(e.what());
  }
  return grpc::Status::OK;
}
grpc::Status RpcFileSystemServer::GetHash(grpc::ServerContext* context, const FileInfo* request, FileInfo* response) {
  auto k_path = p_set.getDataRoot() / request->path();
  if (FSys::exists(k_path) && FSys::is_regular_file(k_path)) {
    auto k_str = k_path.generic_string();
    std::lock_guard k_lock{get_mutex(k_path)->mutex()};

    if (!p_cache.Cached(k_str)) {
      p_cache.Put(k_str, std::make_shared<rpc_filesystem::file_hash>(k_path));
    }
    auto k_hash = p_cache.Get(k_str);
    if (!k_hash->valid()) {
      k_hash->undate_hash();
    }
    response->mutable_hash()->set_value(std::move(k_hash->hash()));
  }

  return grpc::Status::OK;
}

rpc_filesystem::file_hash::file_hash(FSys::path path)
    : _path(std::move(path)),
      _size(FSys::file_size(_path)),
      _time(FSys::last_write_time_point(_path)),
      _hash(),
      _mutex() {
  auto str = FSys::file_hash_sha224(_path);
  std::lock_guard k_lock{_mutex};
  _hash = std::move(str);
}
bool rpc_filesystem::file_hash::valid() const {
  std::lock_guard k_lock{_mutex};
  return _size == FSys::file_size(_path) && _time == FSys::last_write_time_point(_path);
}
std::string rpc_filesystem::file_hash::hash() const {
  std::lock_guard k_lock{_mutex};
  return _hash;
}
void rpc_filesystem::file_hash::undate_hash() {
  /// 在更新时先计算hash值然后加锁去更改会更快
  auto k_item = FSys::file_hash_sha224(_path);

  std::lock_guard k_lock{_mutex};
  _size = FSys::file_size(_path);
  _time = FSys::last_write_time_point(_path);
  _hash = std::move(k_item);
}
namespace rpc_filesystem {
file_mutex::file_mutex()
    : _mutex() {
}

std::mutex& file_mutex::mutex() {
  return _mutex;
}
}  // namespace rpc_filesystem
}  // namespace doodle
