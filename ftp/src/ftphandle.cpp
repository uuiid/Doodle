#include "ftphandle.h"
#include <QUrl>


FTPSPACE_S
ftphandle::~ftphandle()
{

}

ftphandle &ftphandle::getFTP()
{
    static ftphandle install;
    return install;
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
}
FTPSPACE_E
