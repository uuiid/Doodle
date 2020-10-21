#pragma once

#include "ftp_global.h"

#include <QObject>


FTPSPACE_S

class FTP_EXPORT ftphandle : public QObject {
 Q_OBJECT
 public:
  ~ftphandle() override;
  ftphandle(const ftphandle &) = delete;
  ftphandle &operator=(const ftphandle &s) = delete;

  static ftphandle &getFTP();
  ftpSessionPtr session(const QString &host,
                        int prot,
                        const QString &name,
                        const QString &password);
 private:
  ftphandle();

};

FTPSPACE_E


