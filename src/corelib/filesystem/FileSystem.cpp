/*
 * @Author: your name
 * @Date: 2020-09-03 09:10:01
 * @LastEditTime: 2020-12-15 20:52:24
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\fileSystem\src\DfileSyntem.cpp
 */

#include "fileSystem.h"
#include <corelib/filesystem/fileSync.h>
#include <corelib/filesystem/Path.h>
#include <corelib/threadPool/ThreadPool.h>
#include <corelib/Exception/Exception.h>
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

DfileSyntem *DfileSyntem::install = nullptr;

DfileSyntem::~DfileSyntem() {
}

DfileSyntem &DfileSyntem::get() {
  if (!install) nullptr_error("not create file system");
  return *install;
}

std::unique_ptr<DfileSyntem> DfileSyntem::create() {
  auto k_install = std::unique_ptr<DfileSyntem>(new DfileSyntem);

  install = k_install.get();
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
  auto option = std::make_shared<fileDowUpdateOptions>();
  option->setlocaPath(localFile);
  option->setremotePath(remoteFile);
  option->setForce(force);
  return upload(option);
}

bool DfileSyntem::upload(const std::shared_ptr<fileDowUpdateOptions> &option) {
  auto time     = std::chrono::system_clock::now();
  auto time_str = date::format("%Y_%m_%d_%H_%M_%S", time);

  //创建线程池多线程复制
  ThreadPool thread_pool{4};
  std::vector<std::future<bool>> result;

  if (fileSys::is_directory(option->locaPath())) {
    auto dregex      = std::regex(option->locaPath().generic_string());
    auto backup_path = option->remotePath().parent_path() / "backup" / time_str;
    //测试是否具有备份路径
    if (option->hasBackup())
      backup_path = option->backupPath() / time_str;

    for (auto &&item : fileSys::recursive_directory_iterator(option->locaPath())) {
      auto targetPath = std::regex_replace(
          item.path().generic_string(), dregex, "");
      auto k_include = true;
      auto k_exclude = false;

      for (auto &&item : option->Include()) {
        k_include &= std::regex_search(targetPath, *item);
      }
      if (fileSys::is_regular_file(item.path())) {
        if (k_include) {
          for (auto &&item : option->Exclude()) {
            k_exclude |= std::regex_search(targetPath, *item);
          }
          if (!k_exclude) {
            auto path            = option->remotePath() / targetPath;
            auto tmp_backup_path = backup_path / targetPath;

            //这里是添加log信号
            auto logstr = item.path().generic_string() + " --> updata -->  " + path.generic_string();

            filelog(logstr);
            result.emplace_back(
                thread_pool.enqueue([=]() -> bool {
                  return updateFile(item.path(), path, false, tmp_backup_path);
                }));
          }
        }
      }
    }

    for (auto &&item : result) {
      item.get();
    }
    return true;
  } else {
    auto backup_path = option->remotePath().parent_path() /
                       "backup" /
                       time_str /
                       option->remotePath().filename();
    return updateFile(option->locaPath(), option->remotePath(), false, backup_path);
  }
}

bool DfileSyntem::down(const dpath &localFile, const dpath &remoteFile, bool force /*=true */) {
  auto tmp_options = std::make_shared<fileDowUpdateOptions>();
  tmp_options->setForce(force);
  tmp_options->setlocaPath(localFile);
  tmp_options->setremotePath(remoteFile);
  return down(tmp_options);
}

bool DfileSyntem::down(const std::shared_ptr<fileDowUpdateOptions> &option) {
  //创建线程池多线程复制
  ThreadPool thread_pool{4};
  auto time     = std::chrono::system_clock::now();
  auto time_str = date::format("%Y_%m_%d_%H_%M_%S", time);

  std::vector<std::future<bool>> result;
  auto serverPath = std::make_shared<FileSystem::Path>(option->remotePath().generic_string());

  if (!serverPath->exists()) return false;
  std::queue<std::shared_ptr<FileSystem::Path>> pathQueue;
  pathQueue.push(serverPath);

  auto dregex      = std::regex(option->remotePath().generic_string());
  auto backup_path = option->remotePath().parent_path() / "backup" / time_str;
  if (option->hasBackup())
    backup_path = option->backupPath() / time_str;

  while (!pathQueue.empty()) {
    if (pathQueue.front()->isDirectory()) {
      auto list = pathQueue.front()->list();
      for (auto &&item : list) {
        pathQueue.push(item);
      }
    } else {
      auto targetPath = std::regex_replace(
          pathQueue.front()->path()->generic_string(),
          dregex,
          "");  //option->locaPath()->generic_string()

      auto k_include = true;
      auto k_exclude = false;
      for (auto &&item : option->Include()) {
        k_include &= std::regex_search(targetPath, *item);
      }
      if (k_include) {
        for (auto &&item : option->Exclude()) {
          k_exclude |= std::regex_search(targetPath, *item);
        }
        if (!k_exclude) {
          auto path = option->locaPath() / targetPath;
          // auto local_modifyTime = fileSys::exists(path) ? boost::posix_time::from_time_t(
          //                                   boost::filesystem::last_write_time(path))
          //                             : boost::posix_time::min_date_time;
          // auto server_time      = pathQueue.front()->modifyTime();

          //这里是添加log信号
          auto str = pathQueue.front()->path()->generic_string() + " --> down --> " + path.generic_string();
          filelog(str);

          if (fileSys::exists(path) && option->hasBackup()) {
            result.emplace_back(
                thread_pool.enqueue([=]() -> bool {
                  updateFile(path, backup_path / targetPath, {});
                  return downFile(path,
                                  *(pathQueue.front()->path()),
                                  option->Force());
                }));
          } else {
            result.emplace_back(
                thread_pool.enqueue([=]() -> bool {
                  return downFile(path,
                                  *(pathQueue.front()->path()),
                                  option->Force());
                }));
          }
        }
      }
    }
    pathQueue.pop();
  }
  for (auto &&item : result) {
    item.get();
  }
  return true;
}

bool DfileSyntem::exists(const dpath &remoteFile) {
  zmq::socket_t socket{*p_context_, zmq::socket_type::req};
  auto result = FileSystem::Path{remoteFile.generic_string()};
  return result.exists();
}

bool DfileSyntem::createDir(const dpath &remoteFile) {
  auto result = FileSystem::Path{remoteFile.generic_string()};
  result.create();
  return true;
}

bool DfileSyntem::createDir(const std::vector<dpath> &paths) {
  //创建线程池多线程复制
  ThreadPool thread_pool{4};
  //收集结果
  std::vector<std::future<bool>> result;

  for (auto &&item : paths) {
    result.emplace_back(
        thread_pool.enqueue([=]() -> bool {
          return createDir(item);
        }));
  }
  auto b_r = true;
  for (auto &&item : result) {
    b_r &= item.get();
  }
  return b_r;
}

std::shared_ptr<std::string> DfileSyntem::readFileToString(const dpath &remoteFile) {
  auto path   = FileSystem::Path{remoteFile.generic_string()};
  auto result = path.open(std::ios_base::in);
  std::stringstream k_stringstream;
  k_stringstream << result->rdbuf();
  auto str = std::make_shared<std::string>(k_stringstream.str());
  return str;
}

bool DfileSyntem::writeFile(const dpath &remoteFile, const std::shared_ptr<std::string> &data) {
  auto path   = FileSystem::Path{remoteFile.generic_string()};
  auto result = path.open(std::ios_base::in | std::ios::binary);
  result->write(data->data(), data->size());
  return true;
}

bool DfileSyntem::copy(const dpath &sourePath, const dpath &trange_path) {
  auto path = FileSystem::Path{sourePath.generic_string()};
  path.copy({trange_path.generic_string()});
  return true;
}

bool DfileSyntem::localCopy(const dpath &sourePath, const dpath &trange_path, bool backup) {
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
