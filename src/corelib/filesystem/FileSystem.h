/*
 * @Author: your name
 * @Date: 2020-09-02 09:59:06
 * @LastEditTime: 2020-12-01 13:50:48
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\fileSystem\src\DfileSyntem.h
 */
#pragma once

#include <corelib/core_global.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <nlohmann/json.hpp>
#include <boost/filesystem.hpp>

#include <boost/signals2.hpp>

#include <shared_mutex>
#include <optional>

// namespace zmq {
// class context_t;
// class socket_t;
// class message_t;
// };  // namespace zmq

DOODLE_NAMESPACE_S
class Path;
class fileDowUpdateOptions;

class CORE_API DfileSyntem {
 public:
  ~DfileSyntem();
  DfileSyntem(const DfileSyntem &) = delete;
  DfileSyntem &operator=(const DfileSyntem &s) = delete;

  static DfileSyntem &get();
  static std::unique_ptr<DfileSyntem> create();
  void session(const std::string &host,
               int prot,
               const std::string &name,
               const std::string &password,
               const std::string &prijectName);

  boost::signals2::signal<void(const std::string &)> filelog;
  boost::signals2::signal<void(const std::string &)> fileStreamLog;

  bool upload(const fileSys::path &localFile, const fileSys::path &remoteFile, bool force = false);
  bool upload(const std::shared_ptr<fileDowUpdateOptions> &option);
  bool down(const fileSys::path &localFile, const fileSys::path &remoteFile, bool force = false);
  bool down(const std::shared_ptr<fileDowUpdateOptions> &option);
  bool exists(const fileSys::path &remoteFile);
  bool createDir(const fileSys::path &remoteFile);
  bool createDir(const std::vector<fileSys::path> &paths);

  std::shared_ptr<std::string> readFileToString(const fileSys::path &remoteFile);
  bool writeFile(const fileSys::path &remoteFile, const std::shared_ptr<std::string> &data);
  bool copy(const fileSys::path &sourePath, const fileSys::path &trange_path);
  static bool localCopy(const fileSys::path &sourePath, const fileSys::path &trange_path, bool backup);

 private:
  static bool removeDir(const fileSys::path &path);

  DfileSyntem();
  static DfileSyntem *install;

  std::string p_host_;
  int p_prot_;

  std::string tmp_host_prot;

  std::string p_name_;
  std::string p_password_;

  std::string p_ProjectName;

  std::shared_mutex mutex_;
};

DOODLE_NAMESPACE_E
