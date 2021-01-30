/*
 * @Author: your name
 * @Date: 2020-09-03 09:10:01
 * @LastEditTime: 2020-12-15 20:52:24
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\fileSystem\src\DfileSyntem.cpp
 */
#include "DfileSyntem.h"
#include <curl/curl.h>

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

#include <loggerlib/Logger.h>
#include <QUrl>
#include <regex>
#include <stdexcept>

#include <fileSystem/system/ftpsession.h>
#include <iostream>
#include <iomanip>
// #include <ctime>

#include <zmq.hpp>
#include <zmq_addon.hpp>

/*保护data里面的宏__我他妈的*/
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include <date/date.h>
/*保护data里面的宏__我他妈的*/

#include <time.h>
DSYSTEM_S
DfileSyntem::~DfileSyntem() { curl_global_cleanup(); }

DfileSyntem &DfileSyntem::get() {
  static DfileSyntem install;
  return install;
}

void DfileSyntem::session(const std::string &host,
                          int prot,
                          const std::string &name,
                          const std::string &password,
                          const std::string &prijectName) {
  p_host_       = host;
  p_prot_       = prot;
  p_name_       = name;
  p_password_   = password;
  p_ProjectName = prijectName;
}

bool DfileSyntem::upload(const dpath &localFile, const dpath &remoteFile) noexcept {
  return copy(localFile, p_ProjectName / remoteFile, true);
}

bool DfileSyntem::down(const dpath &localFile, const dpath &remoteFile) noexcept {
  return copy(p_ProjectName / remoteFile, localFile, false);
}

bool DfileSyntem::exists(const dpath &remoteFile) const noexcept {
  try {
    return boost::filesystem::exists(p_ProjectName / remoteFile);
  } catch (std::exception &e) {
    DOODLE_LOG_INFO(e.what());
    return false;
  }
}

std::unique_ptr<std::fstream> DfileSyntem::open(const dpath &remoteFile, std::ios_base::openmode mode) const noexcept {
  if (!exists(remoteFile)) {
    boost::filesystem::create_directories(p_ProjectName / remoteFile.parent_path());
  }
  auto file = std::unique_ptr<std::fstream>(new boost::filesystem::fstream(p_ProjectName / remoteFile, mode));
  if (!file->is_open()) {
    file->open(remoteFile.generic_string(), mode);
  }
  return file;
}

bool DfileSyntem::copy(const dpath &sourePath, const dpath &trange_path, bool backup) noexcept {
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
    // //使用std
    // std::time_t t = std::time(nullptr);
    // std::tm tm = *std::localtime(&t);

    //使用msvc_x64
    // struct tm k_tm;
    // time_t time_seconds = time(nullptr);
    // localtime_s(&k_tm, &time_seconds);
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
  boost::filesystem::remove_all(path);
  return true;
}
DfileSyntem::DfileSyntem()
    : p_host_(),
      p_prot_(),
      p_name_(),
      p_password_(),
      p_ProjectName(),
      p_context_(std::make_unique<zmq::context_t>(4)) {
  curl_global_init(CURL_GLOBAL_ALL);
};

DSYSTEM_E
