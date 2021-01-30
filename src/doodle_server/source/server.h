#pragma once
#include <doodle_server/DoodleServer_global.h>

//提前声明
#include <nlohmann/json_fwd.hpp>
namespace zmq {
class multipart_t;
}

DOODLE_NAMESPACE_S

enum class fileOptions {
  getInfo      = 0,
  createFolder = 1,
  update       = 2,
  down         = 3,
  rename       = 4,
  list         = 5,
};

/**
 * @brief 
 * *上传时：
 * *root[class] -> 现在基本只有 filesystem
 * *root[function] -> 这里的功能是上面的枚举
 * *在上传时请在后一帧放入二进制数据
 * 
 * *回复时:
 * *root[status] 中存放状态 标准是ok
 * *root[body] 中基本存放主要结果
 * 
 * *注意：
 * *多一帧作为二进制数据， 不放到json中为了快速序列化
 * 
 * *path类需要：
 * *["path"] -> 路径
 * *["project"] -> 项目名称
 */
class Handler {
 public:
  Handler();

  void operator()(zmq::context_t* context);
  // zmq::multipart_t handleMessage(zmq::multipart_t* message);
 private:
  std::string processMessage(zmq::multipart_t* request_message, zmq::multipart_t* reply_message);

  //* root["body"] ->这里放path类的序列化
  void getInfo(nlohmann::json* root, zmq::multipart_t* reply_message);

  //* root["body"] ->这里放path类的序列化
  void createFolder(nlohmann::json* root, zmq::multipart_t* reply_message);

  //* root["body"] ->这里放path类的序列化
  //* 下一个消息帧放二进制数据
  void update(nlohmann::json* root, zmq::multipart_t* request_message, zmq::multipart_t* reply_message);

  //* root["body"] ->这里放path类的序列化
  //* 会多回复一个消息帧用来放数据
  void down(nlohmann::json* root, zmq::multipart_t* reply_message);

  //* root["body"]["source"] ->这里放旧path类的序列化
  //* root["body"]["trange"] ->这里放新path类的序列化
  void rename(nlohmann::json* root, zmq::multipart_t* reply_message);

  //* root["body"] ->这里放path类的序列化
  //* 回复时在root["body"]中会放path的列表
  //* 同时如果是文件的话就会只放一个请求的路径
  void list(nlohmann::json* root, zmq::multipart_t* reply_message);
};

DOODLE_NAMESPACE_E