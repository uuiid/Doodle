/*
 * @Author: your name
 * @Date: 2020-09-03 09:10:01
 * @LastEditTime: 2020-11-26 19:49:34
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\fileSystem\src\DfileSyntem.cpp
 */
#include "DfileSyntem.h"

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

#include <Logger.h>
#include <QUrl>
#include <regex>
#include <stdexcept>

#include "ftpsession.h"

DSYSTEM_S
DfileSyntem::~DfileSyntem() { curl_global_cleanup(); }

DfileSyntem &DfileSyntem::getFTP() {
  static DfileSyntem install;
  return install;
}

ftpSessionPtr DfileSyntem::session(const std::string &host, int prot,
                                   const std::string &name,
                                   const std::string &password) {
  ftpSessionPtr session(new ftpSession());
  session->setInfo(host, prot, name, password);
  return session;
}
bool DfileSyntem::copy(const dpath &sourePath,
                       const dpath &trange_path) noexcept {
  //创建线程池多线程复制
  boost::asio::thread_pool pool(std::thread::hardware_concurrency());
  //验证文件存在
  if (boost::filesystem::exists(trange_path)) return false;
  if (!boost::filesystem::exists(sourePath)) return false;

  if (boost::filesystem::is_regular_file(sourePath)) {
    boost::filesystem::copy_file(sourePath, trange_path);
  } else {
    auto dregex = std::regex(sourePath.generic_string());
    DOODLE_LOG_INFO << sourePath.generic_string().c_str() << "-->"
                    << trange_path.generic_string().c_str();
    for (auto &item :
         boost::filesystem::recursive_directory_iterator(sourePath)) {
      if (boost::filesystem::is_regular_file(item.path())) {
        dpath basic_string = std::regex_replace(
            item.path().generic_string(), dregex, trange_path.generic_string());
        boost::asio::post(pool, [=]() {
          boost::filesystem::create_directories(basic_string.parent_path());
          boost::filesystem::copy_file(item.path(), basic_string);
        });
      }
    }
    pool.join();
  }

  return true;
}
bool DfileSyntem::removeDir(const dpath &path) {
  boost::filesystem::remove_all(path);
  //  if (!boost::filesystem::exists(path)) return true;
  //
  //  DOODLE_LOG_INFO << "remove dir ->>" << path.generic_string().c_str();
  //  for (auto &item : boost::filesystem::recursive_directory_iterator(path)) {
  //    if (boost::filesystem::is_regular_file(item.path())) {
  //      boost::filesystem::remove(item.path());
  //    }
  //  }
  return true;
}
DfileSyntem::DfileSyntem() = default;

DSYSTEM_E
