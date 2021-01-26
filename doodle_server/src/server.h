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

#include <boost/date_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <nlohmann/json.hpp>
DOODLE_NAMESPACE_S

enum class fileOptions {
  createFolder = 0,
  getInfo      = 1,
  update       = 2,
  down         = 3,
};

class Path {
 public:
  Path(std::string& str);
  Path();
  virtual ~Path();

  boost::filesystem::path* path() const;
  void setPath(const std::string& path_str);
  bool exists() const;
  bool isDirectory() const;
  uint64_t size() const;

  void scanningInfo();

  boost::posix_time::ptime modifyTime() const;

  static void to_json(nlohmann::json& j, const Path& p);
  static void from_json(const nlohmann::json& j, Path& p);

 private:
  std::shared_ptr<boost::filesystem::path> p_path;

  bool p_exist;
  bool p_isDir;
  uint64_t p_size;
  boost::posix_time::ptime p_time;
};

class Handler {
 public:
  Handler();

  void operator()(zmq::context_t* context);
};

DOODLE_NAMESPACE_E