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
// #include <boost/network.hpp>
// #include <boost/network/uri.hpp>
#include <boost/regex.hpp>

#include <zmq.hpp>
#include <zmq_addon.hpp>
//这里我们导入文件映射内存类
#include <boost/iostreams/device/mapped_file.hpp>

#include <thread>

DOODLE_NAMESPACE_S
Path::Path(std::string& str)
    : p_path(nullptr),
      p_exist(false),
      p_isDir(false),
      p_size(0),
      p_time(boost::posix_time::min_date_time) {
  p_path = std::make_shared<boost::filesystem::path>(str);
  scanningInfo();
}

Path::Path()
    : p_path(nullptr),
      p_exist(false),
      p_isDir(false),
      p_size(0),
      p_time(boost::posix_time::min_date_time) {
}

Path::~Path() {
}

bool Path::exists() const {
  return p_exist;
}

bool Path::isDirectory() const {
  return p_isDir;
}

uint64_t Path::size() const {
  return p_size;
}

void Path::scanningInfo() {
  p_exist = boost::filesystem::exists(*p_path);
  if (p_exist) {
    p_isDir = boost::filesystem::is_directory(*p_path);
    p_size  = boost::filesystem::file_size(*p_path);
    p_time  = boost::posix_time::from_time_t(boost::filesystem::last_write_time(*p_path));
  }
}

boost::posix_time::ptime Path::modifyTime() const {
  return p_time;
}

void Path::to_json(nlohmann::json& j, const Path& p) {
  // date::parse()
  auto str = boost::posix_time::to_iso_string(p.modifyTime());
  j        = nlohmann::json{
      {"path", p.p_path->generic_string()},
      {"exists", p.exists()},
      {"isDirectory", p.isDirectory()},
      {"size", p.size()},
      {"modifyTime", str}  //
  };
}

void Path::from_json(const nlohmann::json& j, Path& p) {
  auto str = j.at("path").get<std::string>();
  p.p_path = std::make_shared<boost::filesystem::path>(str);
}
boost::filesystem::path* Path::path() const {
  return p_path.get();
}

void Path::setPath(const std::string& path_str) {
  p_path = std::make_shared<boost::filesystem::path>(path_str);
}

Handler::Handler() {
}

void Handler::operator()(zmq::context_t* context) {
  zmq::socket_t socket{*context, zmq::socket_type::router};

  socket.connect(proxy_point);
  while (true) {
    zmq::multipart_t k_request{socket};

    std::cout << "thread id: " << std::this_thread::get_id() << std::endl;
    std::cout << k_request << std::endl;
    zmq::multipart_t k_reply{};
    const auto k_size = k_request.size();
    for (int i = 0; i < k_size; ++i) {
      if (k_request.front().size() != 0) {
        k_reply.push_back(k_request.pop());  //插入地址
      } else {
        k_request.pop();                      //弹出空帧
        k_reply.push_back(zmq::message_t{});  //插入空帧
        break;
      }
    }

    auto str = k_request.pop().to_string();  //转换内容
    nlohmann::json root;
    try {
      root = nlohmann::json::parse(str);
      root.at("class");

    } catch (const std::exception& e) {
      std::cerr << e.what() << '\n';
      root["error"] = e.what();
    }

    k_reply.push_back(zmq::message_t{root.dump()});
    k_reply.send(socket);
  };
}

DOODLE_NAMESPACE_E
