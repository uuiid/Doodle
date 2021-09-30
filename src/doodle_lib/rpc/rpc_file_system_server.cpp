//
// Created by TD on 2021/6/9.
//

#include "rpc_file_system_server.h"

#include <doodle_lib/core/core_set.h>
#include <Logger/logger.h>
#include <lib_warp/protobuf_warp_cpp.h>

namespace doodle {
rpc_filesystem::file_mutex_ptr rpc_file_system_server::get_mutex(const FSys::path& in_path) {
  auto k_str = in_path.generic_string();

  if (!_mutex.Cached(k_str)) {
    _mutex.Put(k_str, new_object<rpc_filesystem::file_mutex>());
  }
  return _mutex.Get(k_str);
}

rpc_file_system_server::rpc_file_system_server()
    : file_system_server::Service(),
      p_set(core_set::getSet()),
      p_cache(
#ifdef NDEBUG
          1024 * 1024 * 10
#else
          10
#endif
          ),
      _mutex(
#ifdef NDEBUG
          1024 * 1024 * 100
#else
          10
#endif
      ) {
}

grpc::Status rpc_file_system_server::get_info(grpc::ServerContext* context, const file_info_server* request, file_info_server* response) {
  FSys::path k_path = p_set.get_data_root() / request->path();
  DOODLE_LOG_DEBUG(fmt::format("get info path: {}", k_path));

  auto k_ex = FSys::exists(k_path);
  if (!k_ex)
    return grpc::Status::OK;
  response->set_exist(k_ex);

  auto k_dir = FSys::is_directory(k_path);
  response->set_is_folder(k_dir);
  if (k_dir)
    return grpc::Status::OK;

  response->set_size(FSys::file_size(k_path));
  auto k_time        = FSys::last_write_time_t(k_path);
  auto k_google_time = google::protobuf::util::TimeUtil::TimeTToTimestamp(k_time);
  response->mutable_update_time()->CopyFrom(k_google_time);

  return grpc::Status::OK;
}

grpc::Status rpc_file_system_server::is_exist(grpc::ServerContext* context, const file_info_server* request, file_info_server* response) {
  FSys::path k_path = p_set.get_data_root() / request->path();
  auto k_ex         = FSys::exists(k_path);
  response->set_exist(k_ex);
  DOODLE_LOG_DEBUG(fmt::format("get exist path: {}", k_path));
  return grpc::Status::OK;
}

grpc::Status rpc_file_system_server::is_folder(grpc::ServerContext* context, const file_info_server* request, file_info_server* response) {
  FSys::path k_path = p_set.get_data_root() / request->path();
  auto k_ex         = FSys::exists(k_path);
  response->set_exist(k_ex);
  if (!k_ex)
    return grpc::Status::OK;

  auto k_dir = FSys::is_directory(k_path);
  response->set_is_folder(k_dir);
  DOODLE_LOG_DEBUG(("get is dir path: {}", k_path));
  return grpc::Status::OK;
}

grpc::Status rpc_file_system_server::get_size(grpc::ServerContext* context, const file_info_server* request, file_info_server* response) {
  FSys::path k_path = p_set.get_data_root() / request->path();
  auto k_ex         = FSys::exists(k_path);
  response->set_exist(k_ex);
  if (!k_ex)
    return grpc::Status::OK;

  auto k_dir = FSys::is_directory(k_path);
  response->set_is_folder(k_dir);
  if (k_dir)
    return grpc::Status::OK;

  response->set_size(FSys::file_size(k_path));

  DOODLE_LOG_DEBUG(fmt::format("get size path: {}", k_path));

  return grpc::Status::OK;
}

grpc::Status rpc_file_system_server::get_timestamp(grpc::ServerContext* context, const file_info_server* request, file_info_server* response) {
  FSys::path k_path = p_set.get_data_root() / request->path();
  auto k_ex         = FSys::exists(k_path);
  response->set_exist(k_ex);
  if (!k_ex)
    return grpc::Status::OK;

  auto k_dir = FSys::is_directory(k_path);
  response->set_is_folder(k_dir);
  if (k_dir)
    return grpc::Status::OK;

  response->set_size(FSys::file_size(k_path));
  auto k_time        = FSys::last_write_time_t(k_path);
  auto k_google_time = google::protobuf::util::TimeUtil::TimeTToTimestamp(k_time);
  response->mutable_update_time()->CopyFrom(k_google_time);

  DOODLE_LOG_DEBUG(fmt::format("get time path: {}", k_path));

  return grpc::Status::OK;
}

grpc::Status rpc_file_system_server::get_list(grpc::ServerContext* context, const file_info_server* request, grpc::ServerWriter<file_info_server>* writer) {
  auto k_root       = p_set.get_data_root();
  FSys::path k_path = p_set.get_data_root() / request->path();

  DOODLE_LOG_DEBUG(fmt::format("list info path: {}", k_path));

  auto k_ex  = FSys::exists(k_path);
  auto k_dir = FSys::is_directory(k_path);
  if (!k_ex || !k_dir) {
    DOODLE_LOG_DEBUG(fmt::format("路径: {} 存在: {} 是目录: {}", k_path, k_ex, k_dir))
    return {
        grpc::StatusCode::CANCELLED, fmt::format("路径: {} 存在: {} 是目录: {}", k_path, k_ex, k_dir)};
  }

  file_info_server k_info{};
  for (const auto& k_it : FSys::directory_iterator{k_path}) {
    k_info.set_path(std::move(k_it.path().lexically_relative(k_root).generic_string()));
    k_info.set_exist(true);
    k_info.set_is_folder(FSys::is_directory(k_it));
    if (!writer->Write(k_info))
      return grpc::Status::CANCELLED;
  }
  return grpc::Status::OK;
}

grpc::Status rpc_file_system_server::download(grpc::ServerContext* context, const file_info_server* request, grpc::ServerWriter<file_stream_server>* writer) {
  FSys::path k_path = p_set.get_data_root() / request->path();
  auto k_ex         = FSys::exists(k_path);
  auto k_dir        = FSys::is_directory(k_path);
  if (!k_ex || k_dir)
    return grpc::Status::CANCELLED;

  DOODLE_LOG_DEBUG(fmt::format("down path: {}", k_path));

  {
    auto k_m = get_mutex(k_path);
    std::lock_guard k_lock{k_m->mutex()};
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

    auto s_size = core_set::get_block_size();
    file_stream_server k_stream{};

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

grpc::Status rpc_file_system_server::upload(grpc::ServerContext* context, grpc::ServerReader<file_stream_server>* reader, file_info_server* response) {
  file_stream_server k_file_stream{};
  reader->Read(&k_file_stream);
  FSys::path k_path = p_set.get_data_root() / k_file_stream.info().path();
  auto k_ex         = FSys::exists(k_path.parent_path());

  DOODLE_LOG_DEBUG(fmt::format("upload path: {}", k_path));

  if (!k_ex)
    FSys::create_directories(k_path.parent_path());

  auto k_dir = FSys::is_directory(k_path);
  if (k_dir)
    return {grpc::StatusCode::CANCELLED, fmt::format("{} is dir", k_path.generic_string())};

  {
    auto k_m = get_mutex(k_path);
    std::lock_guard k_lock{k_m->mutex()};
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

grpc::Status rpc_file_system_server::move(grpc::ServerContext* context,
                                       const file_info_move_server* request,
                                       file_info_server* response) {
  if (!request->has_source()) {
    DOODLE_LOG_WARN("传入参数无效, 必须来源路径")
    return {grpc::StatusCode::CANCELLED, "传入参数无效, 必须来源路径"};
  }

  FSys::path k_s = p_set.get_data_root() / request->source().path();

  if (!FSys::exists(k_s)) {
    DOODLE_LOG_WARN("来源路径不存在 {}", k_s)
    return {grpc::StatusCode::CANCELLED, "来源路径不存在"};
  }

  FSys::path k_t{};

  if (request->has_target())
    k_t = p_set.get_data_root() / request->target().path();
  else
    k_t = p_set.get_cache_root("delete") / k_s.lexically_proximate(p_set.get_cache_root());

  if (!FSys::exists(k_t.parent_path()))
    FSys::create_directories(k_t.parent_path());

  try {
    ///如果目标存在则直接添加时间戳重命名
    if (FSys::exists(k_t)) {
      k_t = FSys::add_time_stamp(k_t);
    }

    auto k_m = get_mutex(k_s);
    std::lock_guard k_lock{k_m->mutex()};
    FSys::rename(k_s, k_t);
    DOODLE_LOG_INFO("{} -move-> {}", k_s, k_t);
  } catch (const FSys::filesystem_error& e) {
    DOODLE_LOG_WARN(e.what());
  }
  return grpc::Status::OK;
}
grpc::Status rpc_file_system_server::get_hash(grpc::ServerContext* context, const file_info_server* request, file_info_server* response) {
  auto k_path = p_set.get_data_root() / request->path();
  if (FSys::exists(k_path) && FSys::is_regular_file(k_path)) {
    auto k_str = k_path.generic_string();
    auto k_m   = get_mutex(k_path);
    std::lock_guard k_lock{k_m->mutex()};

    if (!p_cache.Cached(k_str)) {
      p_cache.Put(k_str, new_object<rpc_filesystem::file_hash>(k_path));
    }
    auto k_hash = p_cache.Get(k_str);
    if (!k_hash->valid()) {
      k_hash->update_hash();
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
void rpc_filesystem::file_hash::update_hash() {
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

decltype(file_mutex::_mutex)& file_mutex::mutex() {
  return _mutex;
}
}  // namespace rpc_filesystem
}  // namespace doodle
