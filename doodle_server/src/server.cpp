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

mappedFile_ptr fileSystem::get(const std::string& project_name, const path_ptr& path) {
  auto path_iter = p_project_roots.find(project_name);
  if (path_iter != p_project_roots.end()) {
    auto k_path = std::make_shared<boost::filesystem::path>(*(path_iter->second) / (*path));

    boost::iostreams::mapped_file_params parameters{k_path->generic_string()};
    parameters.flags = boost::iostreams::mapped_file::mapmode::readonly;
    auto source      = std::make_shared<boost::iostreams::mapped_file_source>(parameters);
    if (!source->is_open()) {
      source->open(parameters);
    }
    return source;
  }
  return nullptr;
}

std::map<std::string, std::string> fileSystem::mata(const std::string& project_name, const path_ptr& path) {
  std::map<std::string, std::string> handle{
      {"Connection", "close"},
      {"Content-Type", "application/octet-streamm"}};
  auto path_iter = p_project_roots.find(project_name);
  if (path_iter != p_project_roots.end()) {
    boost::filesystem::path k_path(*(path_iter->second) / (*path));
    if (boost::filesystem::is_directory(k_path)) {
      handle.insert({"Content-Length", std::to_string(-1)});
    } else {
      handle.insert({"Content-Length", std::to_string(boost::filesystem::file_size(k_path))});
    }
    auto tmp_time = boost::filesystem::last_write_time(k_path);

    auto time = std::chrono::system_clock::from_time_t(tmp_time);
    handle.insert({"modify_time",
                   date::format("%m %d %Y %H:%M:%S", time)});
  } else {
    handle.insert({"Content-Length", "0"});
    handle.insert({"modify_time", "-1"});
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

void connection_Handler::operator()(const std::string& path, Server::connection_ptr conn,
                                    bool server_body) {
  static constexpr std::size_t MAX_INPUT_SIZE = 2 << 15;
  static const boost::regex re(R"(([0-9|a-z|A-Z|/|_]+)[\?]?[\?|\&]\bproject=([^&]+))");
  boost::smatch match;
  if (boost::regex_search(path, match, re)) {
    if (match.size() >= 2) {
      std::string tmp_p(match[1]);
      std::string prj_name(match[2]);
      auto k_path = std::make_shared<boost::filesystem::path>(tmp_p);
      std::cout << k_path->generic_string() << ": " << prj_name << std::endl;
      if (p_fileSystem->has(prj_name, k_path)) {
        conn->set_headers(p_fileSystem->mata(prj_name, k_path));
        conn->set_status(Server::connection::ok);
        if (server_body)
          send_file(p_fileSystem->get(prj_name, k_path), 0, conn);
        return;
      }
    }
  }
  fail(conn);
}

void connection_Handler::send_file(const mappedFile_ptr& mapFile,
                                   off_t offset,
                                   Server::connection_ptr conn) {
  std::size_t adjusted_offset = offset + 4096;
  off_t rightmost_bound       = std::min(mapFile->size(), adjusted_offset);
  auto self                   = this->shared_from_this();
  conn->write(
      boost::asio::const_buffers_1(
          mapFile->data() + offset,
          rightmost_bound + offset),
      [=](boost::system::error_code const& ec) {
        self->handle_chunk(mapFile, rightmost_bound, conn, ec);
      });
}

void connection_Handler::handle_chunk(const mappedFile_ptr& mapFile, off_t offset,
                                      Server::connection_ptr conn,
                                      boost::system::error_code const& ec) {
  assert(offset >= 0);
  if (!ec && static_cast<std::size_t>(offset) < mapFile->size())
    send_file(mapFile, offset, conn);
}

void connection_Handler::fail(Server::connection_ptr conn) {
  static Server::response_header error_header[] = {"Connection", "close"};
  conn->set_status(Server::connection::not_supported);
  conn->set_headers(boost::make_iterator_range(error_header, error_header + 1));
  conn->write("not find file.");
}

/* -------------------------------------------------------------------------- */
/* ---------------------------------- 主要服务器 --------------------------------- */
/* -------------------------------------------------------------------------- */

Handler::Handler(fileSystem_ptr f_ptr) : p_fileSystem(std::move(f_ptr)) {
}
void Handler::operator()(Server::request const& request,
                         const Server::connection_ptr& connection_) {
  std::string k_dest = destination(request);
  if (request.method == "HEAD") {
    auto handle = std::make_shared<connection_Handler>(p_fileSystem);
    (*handle)(k_dest, connection_, false);
  } else if (request.method == "GET") {
    auto handle = std::make_shared<connection_Handler>(p_fileSystem);
    (*handle)(k_dest, connection_, true);
  } else if (request.method == "PUT" || request.method == "POST") {
  } else {
    static Server::response_header error_header[] = {"Connection", "close"};
    connection_->set_status(Server::connection::not_supported);
    connection_->set_headers(boost::make_iterator_range(error_header, error_header + 1));
    connection_->write("Method not supported.");
  }
}
DOODLE_NAMESPACE_E
