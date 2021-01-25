/*
 * @Author: your name
 * @Date: 2020-12-12 19:17:38
 * @LastEditTime: 2020-12-16 11:07:44
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_server\server.cpp
 */

#include "server.h"
#include <boost/filesystem.hpp>
#include <boost/network.hpp>
#include <boost/network/uri.hpp>
#include <boost/regex.hpp>

//这里我们导入文件映射内存类
#include <boost/iostreams/device/mapped_file.hpp>

/*保护data里面的宏__我他妈的*/
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include <date/date.h>
/*保护data里面的宏__我他妈的*/

#include <chrono>
DOODLE_NAMESPACE_S
fileSystem::fileSystem() {
}

fileSystem::~fileSystem() {
}

grpc::Status fileSystem::exist(grpc::ServerContext* context, const filesys::path* request, filesys::path* response) {
  auto k_path = boost::filesystem::path{request->path_str()};
  response->set_path_str(request->path_str());
  try {
    auto k_path_exists = boost::filesystem::exists(k_path);
    bool k_path_folder{false};
    uint64_t k_size{0};
    if (k_path_exists) {
      k_path_folder = boost::filesystem::is_directory(k_path);
      k_size        = boost::filesystem::file_size(k_path);
    }
    response->set_exist(k_path_exists);
    response->set_is_folder(k_path_folder);
    response->set_size(k_size);

  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
  }


  return grpc::Status::OK;
}

grpc::Status fileSystem::createFolder(::grpc::ServerContext* context, const filesys::path* request, filesys::path* response) {
  return grpc::Status::OK;
}

grpc::Status fileSystem::rename(::grpc::ServerContext* context, const filesys::path* request, filesys::path* response) {
  return grpc::Status::OK;
}

grpc::Status fileSystem::open(::grpc::ServerContext* context, ::grpc::ServerReaderWriter<filesys::io_stream, filesys::io_stream>* stream) {
  return grpc::Status::OK;
}

grpc::Status fileSystem::copy(::grpc::ServerContext* context, const filesys::copy_info* request, filesys::path* response) {
  return grpc::Status::OK;
}

DOODLE_NAMESPACE_E
