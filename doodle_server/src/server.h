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
//后期希望可以去除
// #include <boost/filesystem.hpp>

DOODLE_NAMESPACE_S

class fileSystem {
 public:
  fileSystem();
  void setPrject(const std::pair<std::string, path_ptr> &project_root);

  bool has(const std::string &project_name, const path_ptr &path);
  bool add(const std::string &project_name, const path_ptr &path);
  path_ptr get(const std::string &project_name, const path_ptr &path);
  std::map<std::string, std::string> mata(const std::string &project_name, const path_ptr &path);

 private:
  std::map<std::string, path_ptr> p_project_roots;
};

class connection_Handler
    : public std::enable_shared_from_this<connection_Handler> {
 public:
  connection_Handler(fileSystem_ptr f_ptr);
  void operator()(const std::string &path, Server::connection_ptr conn,
                  bool server_body);
  void send_file(std::pair<void *, std::size_t> mmaped_region, off_t offset, Server::connection_ptr conn);
  void handle_chunk(std::pair<void *, std::size_t> mmaped_region, off_t offset,
                    Server::connection_ptr conn,
                    boost::system::error_code const &ec);

  void fail(Server::connection_ptr conn);

 private:
  fileSystem_ptr p_fileSystem;
  std::string p_project_name;
};

class Handler {
 public:
  Handler(fileSystem_ptr f_ptr);
  void operator()(Server::request const &req,
                  const Server::connection_ptr &conn);

 private:
  fileSystem_ptr p_fileSystem;
};

DOODLE_NAMESPACE_E