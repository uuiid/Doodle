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

#include <boost/filesystem.hpp>
// #include <boost/regex.hpp>

#include <thread>

DOODLE_NAMESPACE_S

Handler::Handler() {
}

void Handler::operator()(zmq::context_t* context) {
  zmq::socket_t socket{*context, zmq::socket_type::router};

  socket.connect(proxy_point);
  while (true) {
    zmq::multipart_t k_request{socket};  //传入消息
    zmq::multipart_t k_reply{};          //回复消息

    std::cout << k_request << std::endl;
    std::cout << "thread id: " << std::this_thread::get_id() << std::endl;
    //处理消息
    auto str = std::move(processMessage(&k_request, &k_reply));

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
            getInfo(&root, &k_reply);
          } break;
          case fileOptions::createFolder: {
            createFolder(&root, &k_reply);
          } break;
          case fileOptions::down: {
            down(&root, &k_reply);
          } break;
          case fileOptions::update: {
            update(&root, &k_request, &k_reply);
          } break;
          case fileOptions::rename: {
            rename(&root, &k_reply);
          } break;
          case fileOptions::list: {
            list(&root, &k_reply);
          } break;
          default: {
            result["status"] = "error: The command could not be found";
            k_reply.push_back(zmq::message_t{result.dump()});
          } break;
        }
      }
    } catch (const std::exception& e) {
      std::cerr << e.what() << '\n';
      result["status"] = e.what();
      k_reply.push_back(zmq::message_t{result.dump()});
    }
    k_reply.send(socket);
  };
}

std::string Handler::processMessage(zmq::multipart_t* request_message, zmq::multipart_t* reply_message) {
  const auto k_size = request_message->size();
  for (int i = 0; i < k_size; ++i) {
    if (request_message->front().size() != 0) {
      reply_message->push_back(request_message->pop());  //插入地址
    } else {
      request_message->pop();                      //弹出空帧
      reply_message->push_back(zmq::message_t{});  //插入空帧
      break;
    }
  }

  auto str = request_message->pop().to_string();  //转换内容
  return str;
}

void Handler::getInfo(nlohmann::json* root, zmq::multipart_t* reply_message) {
  auto path = (*root)["body"].get<Path>();
  path.scanningInfo();

  nlohmann::json result;
  result["body"]   = path;
  result["status"] = "ok";
  reply_message->push_back(zmq::message_t{result.dump()});
}

void Handler::createFolder(nlohmann::json* root, zmq::multipart_t* reply_message) {
  auto path = (*root)["body"].get<Path>();
  path.createFolder();
  path.scanningInfo();

  nlohmann::json result;
  result["body"]   = path;
  result["status"] = "ok";
  reply_message->push_back(zmq::message_t{result.dump()});
}

void Handler::update(nlohmann::json* root, zmq::multipart_t* request_message, zmq::multipart_t* reply_message) {
  auto path  = (*root)["body"].get<Path>();
  auto start = (*root)["body"]["start"].get<uint64_t>();
  auto end   = (*root)["body"]["end"].get<uint64_t>();

  nlohmann::json result;
  path.scanningInfo();
  result["body"] = path;

  //读取上传数据帧
  auto data = request_message->pop();
  if (path.write((char*)data.data(), end - start, start)) {
    result["status"] = "ok";
  } else {
    result["status"] = "error : File write failed ,The file may be occupied";
  }
  reply_message->push_back(zmq::message_t{result.dump()});
}

void Handler::down(nlohmann::json* root, zmq::multipart_t* reply_message) {
  auto path  = (*root)["body"].get<Path>();
  auto start = (*root)["body"]["start"].get<uint64_t>();
  auto end   = (*root)["body"]["end"].get<uint64_t>();

  nlohmann::json result;
  //扫描文件
  path.scanningInfo();
  result["body"] = path;
  // std::string data{};
  // data.resize(end - start);

  // 创建数据帧
  auto data = zmq::message_t{(uint64_t)(end - start)};
  // 检查是否读取成功 成功的话直接返回
  if (path.read((char*)data.data(), end - start, start)) {
    result["status"] = "ok";
    reply_message->push_back(zmq::message_t{result.dump()});
    reply_message->push_back(std::move(data));
  }     //
  else  //如果失败就不返回结果帧
  {
    result["status"] = "error :file occupied for reading";
    reply_message->push_back(zmq::message_t{result.dump()});
  }
}

void Handler::rename(nlohmann::json* root, zmq::multipart_t* reply_message) {
  auto source = (*root)["body"]["source"].get<Path>();
  auto target = (*root)["body"]["target"].get<Path>();

  nlohmann::json result;
  if (source.rename(target)) {
    result["status"] = "ok";
  } else {
    result["status"] = "error: not rename";
  }
  result["body"] = source;
  reply_message->push_back(zmq::message_t{result.dump()});
}

void Handler::list(nlohmann::json* root, zmq::multipart_t* reply_message) {
  auto path = (*root)["body"].get<Path>();
  path.scanningInfo();
  auto list = path.list();

  nlohmann::json result;
  if (list.has_value())
    for (auto&& item : list.value()) {
      result["body"].push_back(*item);
    }
  else
    result["body"].push_back(path);

  result["status"] = "ok";
  reply_message->push_back(zmq::message_t{result.dump()});
}
DOODLE_NAMESPACE_E
