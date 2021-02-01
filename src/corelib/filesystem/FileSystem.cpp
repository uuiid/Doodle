/*
 * @Author: your name
 * @Date: 2020-09-03 09:10:01
 * @LastEditTime: 2020-12-15 20:52:24
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\fileSystem\src\DfileSyntem.cpp
 */

#include "fileSystem.h"
#include <corelib/filesystem/Path.h>
#include <magic_enum.hpp>

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <nlohmann/json.hpp>
#include <loggerlib/Logger.h>

#include <zmq.hpp>
#include <zmq_addon.hpp>

// /*保护data里面的宏__我他妈的*/
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include <date/date.h>
// /*保护data里面的宏__我他妈的*/
#include <stdexcept>
#include <regex>
#include <future>
#include <time.h>
#include <iostream>
#include <queue>

DOODLE_NAMESPACE_S

DfileSyntem::~DfileSyntem() {
  // zsys_shutdown();
}

DfileSyntem &DfileSyntem::get() {
  if (!install) std::runtime_error("not create file system");
  return *install;
}

std::unique_ptr<DfileSyntem> DfileSyntem::create() {
  auto k_install = std::make_unique<DfileSyntem>();
  install        = k_install.get();
  return k_install;
}

void DfileSyntem::session(const std::string &host,
                          int prot,
                          const std::string &name,
                          const std::string &password,
                          const std::string &prijectName) {
  std::unique_lock<std::shared_mutex> lock(mutex_);

  p_host_       = host;
  p_prot_       = prot;
  p_name_       = name;
  p_password_   = password;
  p_ProjectName = prijectName;

  boost::format str{"tcp://%s:%d"};
  str % host % prot;
  tmp_host_prot = str.str();
}

bool DfileSyntem::upload(const dpath &localFile, const dpath &remoteFile, bool force /*=true */) {
  auto time     = std::chrono::system_clock::now();
  auto time_str = date::format("%Y_%m_%d_%H_%M_%S", time);


  //创建线程池多线程复制
  std::queue<std::future<bool>> queue;

  if (fileSys::is_directory(localFile)) {
    auto dregex      = std::regex(localFile.generic_string());
    auto backup_path = remoteFile.parent_path() / "backup" / time_str;
    for (auto &&item : fileSys::recursive_directory_iterator(localFile)) {
      auto targetPath = std::regex_replace(
          item.path().generic_string(), dregex, remoteFile.generic_string());

      if (fileSys::is_regular_file(item.path())) {
        auto fun = std::async(std::launch::async, [=]() -> bool {
          return updateFile(item.path(), targetPath, false, backup_path);
        });
        queue.push(std::move(fun));
        if (queue.size() > 4) {
          queue.front().wait();
          queue.pop();
        }
      }
    }
    while (!queue.empty()) {
      queue.front().wait();
      queue.pop();
    }
    return true;
  } else {
    auto backup_path = remoteFile.parent_path() / "backup" / time_str / remoteFile.filename();
    return updateFile(localFile, remoteFile, false, backup_path);
  }
}

bool DfileSyntem::down(const dpath &localFile, const dpath &remoteFile, bool force /*=true */) {
  //创建线程池多线程复制
  std::queue<std::future<bool>> threadpool;

  zmq::socket_t socket{*p_context_, zmq::socket_type::req};
  nlohmann::json root;
  zmq::multipart_t k_muMsg{};
  std::string project_name{};
  {
    std::shared_lock<std::shared_mutex> lock{mutex_};
    socket.connect(tmp_host_prot);
    project_name = p_ProjectName;
  }

  auto serverPath = getInfo(&remoteFile);
  std::queue<Path> pathQueue;
  Path tmp_ = *serverPath;
  pathQueue.push(tmp_);

  auto dregex = std::regex(remoteFile.generic_string());
  while (!pathQueue.empty()) {
    if (pathQueue.front().isDirectory()) {
      root.clear();
      root["class"]           = "filesystem";
      root["function"]        = magic_enum::enum_name(fileOptions::list);
      root["body"]["path"]    = pathQueue.front().path()->generic_string();
      root["body"]["project"] = project_name;

      k_muMsg.push_back(std::move(zmq::message_t{root.dump()}));
      k_muMsg.send(socket);

      k_muMsg.recv(socket);
      root = nlohmann::json::parse(k_muMsg.pop().to_string());
      for (auto &&item : root["body"]) {
        pathQueue.push(item.get<Path>());
      }
    } else {
      auto targetPath = std::regex_replace(
          pathQueue.front().path()->generic_string(), dregex, localFile.generic_string());

      auto fun = std::async(std::launch::async, [=]() -> bool {
        return downFile(targetPath, pathQueue.front().path()->generic_string(), false);
      });
      threadpool.push(std::move(fun));
      if (threadpool.size() > 4) {
        threadpool.front().wait();
        threadpool.pop();
      }
    }
    pathQueue.pop();
  }
  while (!threadpool.empty()) {
    if (threadpool.front().valid()) {
      threadpool.front().wait();
      auto tmp = threadpool.front().get();
    }
    threadpool.pop();
  }
  return true;
}

bool DfileSyntem::exists(const dpath &remoteFile) {
  auto serverPath = getInfo(&remoteFile);
  return serverPath->Exist();
}

bool DfileSyntem::createDir(const dpath &remoteFile) {
  zmq::socket_t socket{*p_context_, zmq::socket_type::req};
  nlohmann::json root;
  std::string prjectName{};
  {
    std::shared_lock<std::shared_mutex> lock{mutex_};
    socket.connect(tmp_host_prot);
    prjectName = p_ProjectName;
  }
  root["class"]           = "filesystem";
  root["function"]        = magic_enum::enum_name(fileOptions::createFolder);
  root["body"]["project"] = prjectName;
  root["body"]["path"]    = remoteFile.generic_string();

  zmq::multipart_t k_muMsg{};
  k_muMsg.push_back(zmq::message_t{root.dump()});
  k_muMsg.send(socket);

  k_muMsg.recv(socket);
  root = nlohmann::json::parse(k_muMsg.pop().to_string());
  if (root["status"] != "ok") return false;
  auto serverPath = root["body"].get<Path>();
  return serverPath.Exist();
}

std::shared_ptr<std::string> DfileSyntem::readFileToString(const dpath &remoteFile) {
  zmq::socket_t socket{*p_context_, zmq::socket_type::req};
  std::string prjectName{};
  auto str = std::make_shared<std::string>();
  {
    std::shared_lock<std::shared_mutex> lock{mutex_};
    socket.connect(tmp_host_prot);
    prjectName = p_ProjectName;
  }
  nlohmann::json root;
  zmq::multipart_t k_muMsg{};

  auto serverPath = getInfo(&remoteFile);
  if (serverPath->Exist() && !serverPath->isDirectory()) {
    auto size = serverPath->size();
    const uint64_t period{size / off};
    for (uint64_t i = 0; i <= period; ++i) {
      auto end = std::min(off * (i + 1), size);
      root.clear();

      root["class"]           = "filesystem";
      root["function"]        = magic_enum::enum_name(fileOptions::down);
      root["body"]["path"]    = remoteFile.generic_string();
      root["body"]["project"] = prjectName;
      root["body"]["start"]   = i * off;
      root["body"]["end"]     = end;
      k_muMsg.push_back(std::move(zmq::message_t{root.dump()}));
      k_muMsg.send(socket);

      k_muMsg.recv(socket);
      root = nlohmann::json::parse(k_muMsg.pop().to_string());
      if (root["status"] != "ok") return false;
      auto data = k_muMsg.pop();
      str->append(data.to_string());
    }
  }
  return str;
}

bool DfileSyntem::writeFile(const dpath &remoteFile, const std::shared_ptr<std::string> &data) {
  zmq::socket_t socket{*p_context_, zmq::socket_type::req};
  std::string prjectName{};
  {
    std::shared_lock<std::shared_mutex> lock{mutex_};
    socket.connect(tmp_host_prot);
    prjectName = p_ProjectName;
  }
  nlohmann::json root;
  zmq::multipart_t k_muMsg{};

  auto serverPath = getInfo(&remoteFile);
  auto size       = data->size();
  const uint64_t period{size / off};
  for (uint64_t i = 0; i <= period; ++i) {
    auto end = std::min(off * (i + 1), size);
    root.clear();

    root["class"]           = "filesystem";
    root["function"]        = magic_enum::enum_name(fileOptions::down);
    root["body"]["path"]    = remoteFile.generic_string();
    root["body"]["project"] = prjectName;
    root["body"]["start"]   = i * off;
    root["body"]["end"]     = end;
    k_muMsg.push_back(std::move(zmq::message_t{root.dump()}));
    k_muMsg.push_back(std::move(zmq::message_t{data->data(), end - (i * off)}));
    k_muMsg.send(socket);

    k_muMsg.recv(socket);
    root = nlohmann::json::parse(k_muMsg.pop().to_string());
    if (root["status"] != "ok") return false;
  }
  return true;
}

bool DfileSyntem::copy(const dpath &sourePath, const dpath &trange_path, bool backup) {
  //创建线程池多线程复制
  boost::asio::thread_pool pool(std::thread::hardware_concurrency());
  //验证文件存在
  // if (boost::filesystem::exists(trange_path)) return false;
  if (!boost::filesystem::exists(sourePath)) return false;
  if (!boost::filesystem::exists(trange_path.parent_path()))
    boost::filesystem::create_directories(trange_path.parent_path());
  dpath backup_path = "";
  dstring time_str  = "";
  if (backup) {
    //使用c++14库
    auto time = std::chrono::system_clock::now();
    // std::stringstream k_stringstream;
    // k_stringstream << std::put_time(time, "%Y_%m_%d_%H_%M_%S");
    time_str    = date::format("%Y_%m_%d_%H_%M_%S", time);
    backup_path = trange_path.parent_path() / "backup" / time_str /
                  trange_path.filename();
  }

  if (boost::filesystem::is_regular_file(sourePath)) {  //复制文件
    if (!boost::filesystem::exists(trange_path.parent_path()))
      boost::filesystem::create_directories(trange_path.parent_path());
    boost::asio::post(pool, [=]() {
      boost::filesystem::copy_file(sourePath, trange_path,
                                   boost::filesystem::copy_option::overwrite_if_exists);
    });

    if (backup) {
      if (!boost::filesystem::exists(backup_path.parent_path())) {
        boost::filesystem::create_directories(backup_path.parent_path());
      }
      boost::asio::post(pool, [=]() {
        boost::filesystem::copy_file(
            sourePath, backup_path,
            boost::filesystem::copy_option::overwrite_if_exists);
      });
    }

  } else {  //复制目录
    auto dregex = std::regex(sourePath.generic_string());
    DOODLE_LOG_INFO(sourePath.generic_string().c_str()
                    << "-->"
                    << trange_path.generic_string().c_str());
    backup_path = trange_path / "backup" / time_str;
    for (auto &item :
         boost::filesystem::recursive_directory_iterator(sourePath)) {
      if (boost::filesystem::is_regular_file(item.path())) {
        dpath basic_string = std::regex_replace(
            item.path().generic_string(), dregex, trange_path.generic_string());
        boost::asio::post(pool, [=]() {
          if (!boost::filesystem::exists(basic_string.parent_path()))
            boost::filesystem::create_directories(basic_string.parent_path());

          boost::filesystem::copy_file(
              item.path(), basic_string,
              boost::filesystem::copy_option::overwrite_if_exists);
        });
        if (backup) {
          dpath basic_backup_path = std::regex_replace(
              item.path().generic_string(), dregex, backup_path.generic_string());
          boost::asio::post(pool, [=]() {
            if (!boost::filesystem::exists(basic_backup_path.parent_path()))
              boost::filesystem::create_directories(basic_backup_path.parent_path());

            boost::filesystem::copy_file(
                item.path(), basic_backup_path,
                boost::filesystem::copy_option::overwrite_if_exists);
          });
        }
      }
    }
  }
  pool.join();

  return true;
}
bool DfileSyntem::removeDir(const dpath &path) {
  throw std::runtime_error("not is function");
  return false;
}

bool DfileSyntem::updateFile(const dpath &localFile, const dpath &remoteFile, bool force, const dpath &backUpPath) {
  zmq::socket_t socket{*p_context_, zmq::socket_type::req};
  std::string prjectName{};
  {
    std::shared_lock<std::shared_mutex> lock{mutex_};
    socket.connect(tmp_host_prot);
    prjectName = p_ProjectName;
  }
  nlohmann::json root;
  zmq::multipart_t k_muMsg{};

  auto serverPath = getInfo(&remoteFile);

  //如果强制是false 并且服务器上存在文件， 并且修改时间服务器大于等于本地 那么直接忽略
  auto modifyTime = boost::posix_time::from_time_t(boost::filesystem::last_write_time(localFile));
  if (!force && serverPath->Exist() && serverPath->modifyTime() >= modifyTime) {
    return true;
  }

  if (serverPath->Exist()) {
    // 在这里移动备份文件
    root.clear();
    root["class"]                     = "filesystem";
    root["function"]                  = magic_enum::enum_name(fileOptions::rename);
    root["body"]["source"]["path"]    = remoteFile.generic_string();
    root["body"]["source"]["project"] = prjectName;
    root["body"]["target"]["path"]    = backUpPath.generic_string();
    root["body"]["target"]["project"] = prjectName;
    k_muMsg.push_back(std::move(zmq::message_t{root.dump()}));
    k_muMsg.send(socket);

    k_muMsg.recv(socket);
    root = nlohmann::json::parse(k_muMsg.pop().to_string());
    if (root["status"] != "ok") return false;
  }

  //===========================
  std::fstream file{localFile.generic_string(), std::ios::in | std::ios::binary};
  if (!file.is_open()) file.open(localFile.generic_string(), std::ios::in | std::ios::binary);
  auto size = boost::filesystem::file_size(localFile);

  const uint64_t period{size / off};
  for (uint64_t i = 0; i <= period; ++i) {
    auto end = std::min(off * (i + 1), size);
    root.clear();
    root["class"]           = "filesystem";
    root["function"]        = magic_enum::enum_name(fileOptions::update);
    root["body"]["path"]    = remoteFile.generic_string();
    root["body"]["project"] = prjectName;
    root["body"]["start"]   = i * off;
    root["body"]["end"]     = end;
    k_muMsg.push_back(std::move(zmq::message_t{root.dump()}));

    auto data = zmq::message_t{end - (i * off)};
    file.seekg(i * off);
    file.read((char *)data.data(), end - (i * off));
    k_muMsg.push_back(std::move(data));

    k_muMsg.send(socket);
    k_muMsg.recv(socket);

    root = nlohmann::json::parse(k_muMsg.pop().to_string());
    if (root["status"] != "ok")
      return false;
  }
  return true;
}

bool DfileSyntem::downFile(const dpath &localFile, const dpath &remoteFile, bool force) {
  zmq::socket_t socket{*p_context_, zmq::socket_type::req};
  std::string prjectName{};
  {
    std::shared_lock<std::shared_mutex> lock{mutex_};
    socket.connect(tmp_host_prot);
    prjectName = p_ProjectName;
  }
  nlohmann::json root;
  zmq::multipart_t k_muMsg{};

  auto serverPath = getInfo(&remoteFile);

  if (!boost::filesystem::exists(localFile.parent_path())) {
    boost::filesystem::create_directories(localFile.parent_path());
  }

  //如果强制是false 并且服务器上存在文件， 并且修改时间服务器小于等于本地 那么直接忽略
  if (boost::filesystem::exists(localFile)) {
    auto modifyTime = boost::posix_time::from_time_t(boost::filesystem::last_write_time(localFile));
    if (!force && serverPath->Exist() && serverPath->modifyTime() <= modifyTime) {
      return true;
    }
  }

  std::fstream file{localFile.generic_string(), std::ios::out | std::ios::binary};
  if (!file.is_open()) file.open(localFile.generic_string(), std::ios::out | std::ios::binary);

  auto size = serverPath->size();
  const uint64_t period{size / off};
  for (uint64_t i = 0; i <= period; ++i) {
    auto end = std::min(off * (i + 1), size);
    root.clear();

    root["class"]           = "filesystem";
    root["function"]        = magic_enum::enum_name(fileOptions::down);
    root["body"]["path"]    = remoteFile.generic_string();
    root["body"]["project"] = prjectName;
    root["body"]["start"]   = i * off;
    root["body"]["end"]     = end;
    k_muMsg.push_back(std::move(zmq::message_t{root.dump()}));
    k_muMsg.send(socket);

    k_muMsg.recv(socket);
    root = nlohmann::json::parse(k_muMsg.pop().to_string());
    if (root["status"] != "ok") return false;
    auto data = k_muMsg.pop();
    file.write((char *)data.data(), data.size());
  }
  return true;
}

std::unique_ptr<Path> DfileSyntem::getInfo(const dpath *path) {
  auto k_path = std::make_unique<Path>();
  zmq::socket_t socket{*p_context_, zmq::socket_type::req};
  nlohmann::json root;
  std::string prjectName{};
  {
    std::shared_lock<std::shared_mutex> lock{mutex_};
    socket.connect(tmp_host_prot);
    prjectName = p_ProjectName;
  }
  root["body"]["project"] = prjectName;
  root["class"]           = "filesystem";
  root["function"]        = magic_enum::enum_name(fileOptions::getInfo);
  root["body"]["path"]    = path->generic_string();

  zmq::multipart_t k_muMsg{};
  k_muMsg.push_back(zmq::message_t{root.dump()});
  k_muMsg.send(socket);

  k_muMsg.recv(socket);
  root = nlohmann::json::parse(k_muMsg.pop().to_string());
  if (root["status"] != "ok") return false;
  *k_path = root["body"].get<Path>();
  return k_path;
}

DfileSyntem::DfileSyntem()
    : p_host_(),
      p_prot_(),
      tmp_host_prot(),
      p_name_(),
      p_password_(),
      p_ProjectName(),
      p_context_(std::make_unique<zmq::context_t>(4)),
      mutex_(){};

DOODLE_NAMESPACE_E
