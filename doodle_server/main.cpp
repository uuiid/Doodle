/*
 * @Author: your name
 * @Date: 2020-12-12 13:21:34
 * @LastEditTime: 2020-12-15 12:00:44
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_server\main.cpp
 */

#include <server.h>
#include <zmq.hpp>
#include <zmq_addon.hpp>

#include <boost/filesystem.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
// #include <boost/network/utils/thread_pool.hpp>

int main(int argc, char const *argv[]) try {
  const std::string endpoint = R"(tcp://*:6666)";

  zmq::context_t context{32};
  zmq::socket_t socket{context, zmq::socket_type::rep};
  socket.bind(endpoint);

  //文件处理类
  auto fileSys = std::make_shared<doodle::fileSystem>();
  while (true) {
    zmq::message_t message;
    socket.recv(message, zmq::recv_flags::none);  //zmq::recv_flags::dontwait
    std::cout << message << std::endl;

    // message.to_string();
    auto p_path = std::make_shared<boost::filesystem::path>(message.to_string());
    if (fileSys->has("dubuxiaoyao3", p_path)) {
      auto file = fileSys->get("dubuxiaoyao3", p_path);
      boost::filesystem::ifstream stream(*file, std::ifstream::in | std::ifstream::binary);

      boost::iostreams::mapped_file_params parameters{file->generic_string()};
      parameters.flags = boost::iostreams::mapped_file::mapmode::readonly;

      boost::iostreams::mapped_file_source source{parameters};
      if (!source.is_open())
        source.open(parameters);

      std::cout << "da xiao " << source.size() << std::endl;
      // std::cout.write(source.data(), source.size());
      zmq::message_t p_message{(void *)source.data(), source.size()};
      // p_message
      socket.send(p_message, zmq::send_flags::none);
    } else {
      socket.send(message, zmq::send_flags::none);
    }
  }

  // auto fileSys = std::make_shared<doodle::fileSystem>();
  // doodle::Handler handle(fileSys);
  // doodle::Server::options options(handle);
  // doodle::Server instance{
  //     options.thread_pool(std::make_shared<boost::network::utils::thread_pool>(4))
  //         .address("127.0.0.1")
  //         .port("8000")};
  // instance.run();
  return 0;
} catch (const std::exception &error) {
  std::cout << error.what() << std::endl;
  return 1;
}
