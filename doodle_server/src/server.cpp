/*
 * @Author: your name
 * @Date: 2020-12-12 19:17:38
 * @LastEditTime: 2020-12-14 10:42:06
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_server\server.cpp
 */

#include "server.h"
#include <boost/filesystem.hpp>
DOODLE_NAMESPACE_S

fileSystem::fileSystem()
    : p_project_roots() {
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

bool fileSystem::get(const std::string& project_name, const path_ptr& path) {
  return false;
}

bool fileSystem::mata(const std::string& project_name, const path_ptr& path) {
  return false;
}

void fileSystem::setPrject(const std::pair<std::string, path_ptr>& project_root) {
  p_project_roots.insert(project_root);
}

void connection_Handler::operator()(const std::string& path, Server::connection_ptr conn,
                                    bool server_body) {
  auto k_path = boost::filesystem::path(path);
  if (boost::filesystem::exists(k_path)) {
  }
}
void Handler::operator()(Server::request const& req,
                         const Server::connection_ptr& conn) {
  if (req.method == "HEAD") {
  }
}
DOODLE_NAMESPACE_E
