#include "ftpsession.h"

#include <fstream>


FTPSPACE_S
ftpSession::ftpSession()
{
    ftp = QSharedPointer<QNetworkAccessManager>(new QNetworkAccessManager(this));
    ptrUrl = QSharedPointer<QUrl>(new QUrl());
    outfile = QSharedPointer<QFile>(new QFile());
    inputfile = QSharedPointer<QFile>(new QFile());
}

ftpSession::~ftpSession()
{
}

void ftpSession::setInfo(const QString &host,int prot, const QString &name, const QString &password)
{
    ptrUrl->setScheme("ftp");
    ptrUrl->setHost(host);
    ptrUrl->setPort(prot);
    if(!name.isEmpty()){
        ptrUrl->setUserName(name);
    }else {
        ptrUrl->setUserName("anonymous");
    }
    if(!password.isEmpty()) {
        ptrUrl->setPassword(password);
    }else{
        ptrUrl->setPassword("");
    }
}

void ftpSession::down(const QString &localFile, const QString &remoteFile)
{
    outfile->setFileName(localFile);
    if(!outfile->open(QIODevice::WriteOnly)){
        throw std::runtime_error("not open file");
        return ;
    }
    ptrUrl->setPath(remoteFile);
    QNetworkRequest request(*ptrUrl);
    currentDownload = ftp->get(request);
    connect(currentDownload, &QNetworkReply::finished,
            this,            &ftpSession::downloadFinished);
    connect(currentDownload, &QNetworkReply::readyRead,
            this,            &ftpSession::WriteFileCallbask);

}

void ftpSession::updata(const QString &localFile, const QString &remoteFile)
{
    inputfile->setFileName(localFile);
    if(!inputfile->open(QIODevice::ReadOnly)){
        throw std::runtime_error("not open file(Read Only)");
        return ;
    }
    ptrUrl->setPath(remoteFile);
    QNetworkRequest requst(*ptrUrl);
    currentUpdata = ftp->put(requst,inputfile.get());
    connect(currentUpdata, &QNetworkReply::finished,
            this,          &ftpSession::updataFinished);
}

void ftpSession::WriteFileCallbask()
{
    outfile->write(currentDownload->readAll());
}


void ftpSession::downloadFinished()
{
    outfile->close();
    if (currentDownload->error()){
        qDebug()<<"down file err:" <<currentDownload->errorString();
    }
    currentDownload->deleteLater();
    emit finished();
}

void ftpSession::updataFinished()
{
    inputfile->close();
    if(currentUpdata->error()){
        qDebug()<<"upload file err:" << currentUpdata->errorString();
    }
    currentUpdata->deleteLater();
    emit finished();
}
FTPSPACE_E
