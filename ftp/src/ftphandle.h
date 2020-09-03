#ifndef FTPHANDLE_H
#define FTPHANDLE_H

#include "ftp_global.h"
#include "ftpsession.h"
#include <QObject>
#include <QSharedPointer>
#include <QFile>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
FTPSPACE_S

class FTP_EXPORT ftphandle:public QObject
{
    Q_OBJECT
public:
    ~ftphandle();
    ftphandle(const ftphandle&) =delete ;
    ftphandle& operator =(const ftphandle& s) =delete ;

    static ftphandle& getFTP();
    ftpSession session(const QString &host,
                       const qint16 &prot = 21,
                       const QString &name = "",
                       const QString &password="");
    void downfile(const QUrl & url, const QString &outFile_);


protected slots:
    void downloadReadyRead();
    void downloadFinished();
    void merror(QNetworkReply::NetworkError err);

protected:
    QSharedPointer<QNetworkAccessManager> ftp;
    QNetworkReply *currentDownload = nullptr;
    QFile outfile;

private:
    ftphandle();

};

FTPSPACE_E

#endif // FTPHANDLE_H
