/*
 * @Author: your name
 * @Date: 2020-12-12 19:17:38
 * @LastEditTime: 2020-12-16 11:07:44
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_server\server.cpp
 */

#include "server.h"
// #include <boost/network.hpp>
// #include <boost/network/uri.hpp>
#include <share/path/path.h>
#include <magic_enum.hpp>
#include <zmq.hpp>
#include <zmq_addon.hpp>
//这里我们导入文件映射内存类
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/iostreams/device/mapped_file.hpp>

#include <thread>

DOODLE_NAMESPACE_S

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
    nlohmann::json result;
    try {
      root           = nlohmann::json::parse(str);
      auto class_str = root.at("class").get<std::string>();
      auto fun_str   = root.at("function").get<std::string>();
      if (class_str == "filesystem") {
        auto fun = magic_enum::enum_cast<fileOptions>(fun_str).value_or(fileOptions::getInfo);
        switch (fun) {
          case fileOptions::getInfo: {
            auto path = root["body"].get<Path>();
            path.scanningInfo();

            result["body"]  = path;
            result["error"] = "ok";
          } break;
          case fileOptions::createFolder: {
            auto path = root["body"].get<Path>();
            path.scanningInfo();
            path.createFolder();

            result["body"]  = path;
            result["error"] = "ok";
          } break;
          default:
            break;
        }
      }

    } catch (const std::exception& e) {
      std::cerr << e.what() << '\n';
      result["error"] = e.what();
    }

    k_reply.push_back(zmq::message_t{result.dump()});
    k_reply.send(socket);
  };
}

DOODLE_NAMESPACE_E
