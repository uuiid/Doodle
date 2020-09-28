#pragma once

#include "ftp_global.h"

#include "ftphandle.h"

#include <curl/curl.h>
#include <QtCore>
#include <QObject>
#include <QSharedPointer>
#include <QUrl>
#include <QFile>

FTPSPACE_S
struct oFileInfo
{
    QString filepath;
    bool isFolder;
    time_t fileMtime;
    double fileSize;
};

class FTP_EXPORT ftpSession : public QObject
{
    Q_OBJECT

public:
    ~ftpSession();

    bool down(const QString &localFile, const QString &remoteFile);
    bool upload(const QString &localFile, const QString &remoteFile);
    oFileInfo fileInfo(const QString &remoteFile);
    //获得文件列表
    std::vector<oFileInfo> list(const QString &remoteFolder);

    friend ftpSessionPtr ftphandle::session(const QString &host,
                                            int prot,
                                            const QString &name,
                                            const QString &password);
signals:
    void finished();

private:
    ftpSession();
    void setInfo(const QString &host,
                 int prot,
                 const QString &name,
                 const QString &password);
    static size_t writeFileCallbask(void *buff, size_t size, size_t nmemb, void *data);
    static size_t readFileCallbask(void *buff, size_t size, size_t nmemb, void *data);
    static size_t notCallbask(void *buff, size_t size, size_t nmemb, void *data);
    static size_t writeStringCallbask(void *ptr, size_t size, size_t nmemb, void *data);
    CURLcode perform();

private:
    QSharedPointer<QFile> outfile;
    QSharedPointer<QFile> inputfile;
    QSharedPointer<QUrl> ptrUrl;

    mutable CURL *curlSession;
};

FTPSPACE_E
