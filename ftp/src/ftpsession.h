#ifndef FTPSESSION_H
#define FTPSESSION_H

#include "ftp_global.h"
#include <QtCore>
#include <QObject>
#include <QSharedPointer>
#include <QUrl>
#include <QFile>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

FTPSPACE_S
class FTP_EXPORT ftpSession:public QObject
{
    Q_OBJECT
public:
    ftpSession();
    ~ftpSession();
    void setInfo(const QString &host,
                 int prot,
                 const QString &name,
                 const QString &password);

    void down  (const QString& localFile,const QString& remoteFile);
    void updata(const QString& localFile,const QString& remoteFile);
signals:
    void finished();

private slots:
    void WriteFileCallbask();
    void downloadFinished();

    void updataFinished();

private:
    QSharedPointer<QFile> outfile;
    QSharedPointer<QFile> inputfile;
    QSharedPointer<QUrl> ptrUrl;
    QSharedPointer<QNetworkAccessManager> ftp;
    QNetworkReply *currentDownload = nullptr;
    QNetworkReply *currentUpdata   = nullptr;
};
typedef QSharedPointer<ftpSession> ftpSessionPtr;
FTPSPACE_E
#endif // FTPSESSION_H
