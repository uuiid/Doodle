#pragma once

#include <fileSystem_global.h>

DSYSTEM_S

class DSYSTEM_API DfileSyntem{

 public:
  ~DfileSyntem();
  DfileSyntem(const DfileSyntem &) = delete;
  DfileSyntem &operator=(const DfileSyntem &s) = delete;

  static DfileSyntem &getFTP();
  ftpSessionPtr session(const std::string &host,
                        int prot,
                        const std::string &name,
                        const std::string &password);

  static bool copy(const dpath& sourePath,const  dpath& trange_path) noexcept;
  static bool removeDir(const dpath &path);
 private:
  DfileSyntem();

};

DSYSTEM_E


