#pragma once

#include "ftp_global.h"

#include <QObject>


FTPSPACE_S

class FTP_EXPORT ftphandle{

 public:
  ~ftphandle();
  ftphandle(const ftphandle &) = delete;
  ftphandle &operator=(const ftphandle &s) = delete;

  static ftphandle &getFTP();
  ftpSessionPtr session(const std::string &host,
                        int prot,
                        const std::string &name,
                        const std::string &password);
 private:
  ftphandle();

};

FTPSPACE_E


