/*
 * @Author: your name
 * @Date: 2020-12-12 19:17:30
 * @LastEditTime: 2020-12-14 19:32:14
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_server\server.h
 */

#pragma once
#include <DoodleServer_global.h>

DOODLE_NAMESPACE_S
class fileSystem {
 public:
  fileSystem();
  void setPrject(const std::pair<std::string, path_ptr> &project_root);

  bool has(const std::string &project_name, const path_ptr &path);
  bool add(const std::string &project_name, const path_ptr &path);
  bool get(const std::string &project_name, const path_ptr &path);
  bool mata(const std::string &project_name, const path_ptr &path);

 private:
  std::map<std::string, path_ptr> p_project_roots;
};

class connection_Handler
    : public std::enable_shared_from_this<connection_Handler> {
 public:
  connection_Handler(file_system_ptr);
  void operator()(const std::string &path, Server::connection_ptr conn,
                  bool server_body);
};

class Handler {
 public:
  void operator()(Server::request const &req,
                  const Server::connection_ptr &conn);
};

DOODLE_NAMESPACE_E