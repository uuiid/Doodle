/*
 * @Author: your name
 * @Date: 2020-09-03 09:10:01
 * @LastEditTime: 2020-11-29 17:27:56
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\fileSystem\src\DfileSyntem.cpp
 */
#include "DfileSyntem.h"
#include <curl/curl.h>

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

#include <Logger.h>
#include <QUrl>
#include <regex>
#include <stdexcept>

#include <src/ftpsession.h>

DSYSTEM_S
DfileSyntem::~DfileSyntem() { curl_global_cleanup(); }

DfileSyntem &DfileSyntem::getFTP() {
  static DfileSyntem install;
  return install;
}

ftpSessionPtr DfileSyntem::session() const {
  auto session = std::make_shared<ftpSession>();
  session->setInfo(p_host_, p_prot_, p_name_, p_password_);
  return session;
}

ftpSessionPtr DfileSyntem::session(const std::string &host, int prot,
                                   const std::string &name,
                                   const std::string &password) {
  ftpSessionPtr session(new ftpSession());
  p_host_ = host;
  p_prot_ = prot;
  p_name_ = name;
  p_password_ = password;

  session->setInfo(p_host_, p_prot_, p_name_, p_password_);
  return session;
}

bool DfileSyntem::copy(const dpath &sourePath,
                       const dpath &trange_path) noexcept {
  //创建线程池多线程复制
  boost::asio::thread_pool pool(std::thread::hardware_concurrency());
  //验证文件存在
  // if (boost::filesystem::exists(trange_path)) return false;
  if (!boost::filesystem::exists(sourePath)) return false;
  if (!boost::filesystem::exists(trange_path.parent_path()))
    boost::filesystem::create_directories(trange_path.parent_path());

  if (boost::filesystem::is_regular_file(sourePath)) {
    boost::filesystem::copy_file(
        sourePath, trange_path,
        boost::filesystem::copy_option::overwrite_if_exists);
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
          if (!boost::filesystem::exists(basic_string.parent_path()))
            boost::filesystem::create_directories(basic_string.parent_path());

          boost::filesystem::copy_file(
              item.path(), basic_string,
              boost::filesystem::copy_option::overwrite_if_exists);
        });
      }
    }
    pool.join();
  }

  return true;
}
bool DfileSyntem::removeDir(const dpath &path) {
  boost::filesystem::remove_all(path);
  return true;
}
DfileSyntem::DfileSyntem() : p_host_(), p_prot_(), p_name_(), p_password_() {
  curl_global_init(CURL_GLOBAL_ALL);
};

DSYSTEM_E
