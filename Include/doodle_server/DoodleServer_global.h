/*
 * @Author: your name
 * @Date: 2020-12-12 19:25:24
 * @LastEditTime: 2020-12-14 10:19:35
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_server\DoodleServer.h
 */

#pragma once
#include <string>
#include <memory>
#include <map>
// #include <boost/network/include/http/server.hpp>
#include <filesystem>
namespace boost {

namespace filesystem {
class path;
};

};  // namespace boost

namespace zmq {
class context_t;
}

#define DOODLE doodle
#define DOODLE_NAMESPACE_S namespace DOODLE {
#define DOODLE_NAMESPACE_E \
  }                        \
  ;

DOODLE_NAMESPACE_S
namespace fileSys                    = boost::filesystem;
const static std::string endpoint    = R"(tcp://*:6666)";
const static std::string proxy_point = R"(tcp://127.0.0.1:6667)";

DOODLE_NAMESPACE_E