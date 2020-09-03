#ifndef FTPSESSION_H
#define FTPSESSION_H

#include "ftp_global.h"
#include <curl/curl.h>
#include <QSharedPointer>
#include <QUrl>

FTPSPACE_S
class FTP_EXPORT ftpSession
{
public:
    ftpSession();
    ~ftpSession();
    void setInfo(const QString &host,
                 const qint16 &prot = 21,
                 const QString &name = "",
                 const QString &password="");

    bool down(const QString& localFile,const QString& remoteFile);

private:
    mutable CURL * ptrSession;
    QSharedPointer<QUrl> ptrUrl;
private:
    size_t WriteToFileCallbask(void *buff,
                               size_t size,
                               size_t nmemb,
                               void *data);
    CURLcode perfrom() const;
};
FTPSPACE_E
#endif // FTPSESSION_H
