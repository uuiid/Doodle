#include "ftphandle.h"
#include <QUrl>
#include <curl/curl.h>
#include <stdexcept>

FTPSPACE_S
ftphandle::~ftphandle()
{
    curl_global_cleanup();
}

ftphandle &ftphandle::getFTP()
{
    static ftphandle install;
    return install;
}

ftpSession ftphandle::session(const QString &host,const qint16 &prot,const QString &name,const QString &password)
{
    ftpSession session;
    session.setInfo(host,prot,name,password);
    return session;
}

void ftphandle::downfile(const QUrl & url,const QString & outFile_)
{
    outfile.setFileName(outFile_);
    if(!outfile.open(QIODevice::WriteOnly)){
        throw std::runtime_error("not open file");
        return ;
    }
    QNetworkRequest request(url);
    currentDownload = ftp->get(request);
    connect(currentDownload, &QNetworkReply::finished,
            this,            &ftphandle::downloadFinished);
    connect(currentDownload, &QNetworkReply::readyRead,
            this,            &ftphandle::downloadReadyRead);
}

void ftphandle::downloadReadyRead()
{
    outfile.write(currentDownload->readAll());
}

void ftphandle::downloadFinished()
{
    outfile.close();
    if (currentDownload->error()){
        qDebug()<<"down file err:" <<currentDownload->errorString();
    }
    currentDownload->deleteLater();
}

void ftphandle::merror(QNetworkReply::NetworkError err)
{
    qDebug() << err;
}

ftphandle::ftphandle()
{
    ftp = QSharedPointer<QNetworkAccessManager>(new QNetworkAccessManager(this));
    const auto  eCode = curl_global_init(CURL_GLOBAL_ALL);
    if(eCode != CURLE_OK){
        throw std::runtime_error("Error initializing libCURL");
    }
}
FTPSPACE_E
