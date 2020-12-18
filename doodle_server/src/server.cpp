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

//文件映射类
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
#include <zmq_addon.hpp>

DOODLE_NAMESPACE_S

fileSystem::fileSystem()
    : p_project_roots() {
  p_project_roots.insert({"dubuxiaoyao3", std::make_shared<boost::filesystem::path>("D:/")});
}

bool fileSystem::has(const std::string& project_name, const path_ptr& path) {
  auto path_iter = p_project_roots.find(project_name);
  if (path_iter != p_project_roots.end())
    return boost::filesystem::exists(*(path_iter->second) / (*path));
  else
    return false;
}

bool fileSystem::add(const std::string& project_name, const path_ptr& path) {
  return false;
}

path_ptr fileSystem::get(const std::string& project_name, const path_ptr& path) {
  auto path_iter = p_project_roots.find(project_name);
  if (path_iter != p_project_roots.end()) {
    auto k_path = std::make_shared<boost::filesystem::path>(*(path_iter->second) / (*path));
    boost::filesystem::ifstream stream(*k_path, std::ios::in | std::ios::binary);
    if (stream.is_open()) {
      return k_path;
    }
  }
  return nullptr;
}

std::map<std::string, std::string> fileSystem::mata(const std::string& project_name, const path_ptr& path) {
  std::map<std::string, std::string> handle{};
  auto path_iter = p_project_roots.find(project_name);
  if (path_iter != p_project_roots.end()) {
    boost::filesystem::path k_path(*(path_iter->second) / (*path));
    if (boost::filesystem::is_directory(k_path)) {
      handle.insert({"file_size", std::to_string(-1)});
    } else {
      handle.insert({"file_size", std::to_string(boost::filesystem::file_size(k_path))});
    }
    auto tmp_time = boost::filesystem::last_write_time(k_path);

    auto time = std::chrono::system_clock::from_time_t(tmp_time);
    handle.insert({"modify_time",
                   date::format("%m %d %Y %H:%M:%S", time)});
  } else {
    handle.insert({"file_size", ""});
    handle.insert({"modify_time", ""});
  }
  return handle;
}

void fileSystem::setPrject(const std::pair<std::string, path_ptr>& project_root) {
  p_project_roots.insert(project_root);
}

/* -------------------------------------------------------------------------- */
/* ---------------------------------连接处理---------------------------------- */
/* -------------------------------------------------------------------------- */

connection_Handler::connection_Handler(fileSystem_ptr f_ptr)
    : p_fileSystem(std::move(f_ptr)), p_project_name() {}

void connection_Handler::operator()(const std::string& path, sokcket_ptr conn,
                                    bool server_body) {
}

void connection_Handler::send_file(std::pair<void*, std::size_t> mmaped_region,
                                   off_t offset,
                                   sokcket_ptr conn) {
}

void connection_Handler::handle_chunk(std::pair<void*, std::size_t> mmaped_region, off_t offset,
                                      sokcket_ptr conn,
                                      boost::system::error_code const& ec) {
}

void connection_Handler::fail(sokcket_ptr conn) {
}

/* -------------------------------------------------------------------------- */
/* ---------------------------------- 主要服务器 --------------------------------- */
/* -------------------------------------------------------------------------- */

Handler::Handler(fileSystem_ptr f_ptr) : p_fileSystem(std::move(f_ptr)) {
}
void Handler::operator()(zmq::context_t* context) {
  zmq::socket_t socket(*context, zmq::socket_type::rep);
  socket.connect(proxy_point);
  while (true) {
    zmq::multipart_t request{};
    request.recv(socket);
    auto p_path = std::make_shared<boost::filesystem::path>(request.back().to_string());

    if (p_fileSystem->has("dubuxiaoyao3", p_path)) {
      zmq::multipart_t reply{};

      auto file = p_fileSystem->get("dubuxiaoyao3", p_path);
      boost::filesystem::ifstream stream(*file, std::ifstream::in | std::ifstream::binary);

      boost::iostreams::mapped_file_params parameters{file->generic_string()};
      parameters.flags = boost::iostreams::mapped_file::mapmode::readonly;

      boost::iostreams::mapped_file_source source{parameters};
      if (!source.is_open())
        source.open(parameters);

      std::cout << "da xiao " << source.size() << std::endl;
      //在这个地方我们构造消息的主体
      zmq::message_t k_reply{(void*)source.data(), source.size()};
      reply.add(std::move(k_reply));
      reply.send(socket);
    } else {
      request.send(socket);
    }
  }
}
DOODLE_NAMESPACE_E
