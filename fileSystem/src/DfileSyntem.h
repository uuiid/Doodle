/*
 * @Author: your name
 * @Date: 2020-09-02 09:59:06
 * @LastEditTime: 2020-11-29 16:11:11
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\fileSystem\src\DfileSyntem.h
 */
#pragma once

#include <fileSystem_global.h>

DSYSTEM_S

class DSYSTEM_API DfileSyntem {
 public:
  ~DfileSyntem();
  DfileSyntem(const DfileSyntem &) = delete;
  DfileSyntem &operator=(const DfileSyntem &s) = delete;

  static DfileSyntem &getFTP();
  ftpSessionPtr session() const;
  ftpSessionPtr session(const std::string &host, int prot,
                        const std::string &name, const std::string &password);

  static bool copy(const dpath &sourePath, const dpath &trange_path) noexcept;
  static bool removeDir(const dpath &path);

 private:
  DfileSyntem();
  std::string p_host_;
  int p_prot_;
  std::string p_name_;
  std::string p_password_;
};

DSYSTEM_E
