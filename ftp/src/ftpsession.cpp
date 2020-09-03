#include "ftpsession.h"
#include <QFile>
#include <fstream>


FTPSPACE_S
ftpSession::ftpSession()
{
    ptrSession = curl_easy_init();
    ptrUrl = QSharedPointer<QUrl>(new QUrl);
}

ftpSession::~ftpSession()
{
    curl_easy_cleanup(ptrSession);
}

void ftpSession::setInfo(const QString &host, const qint16 &prot, const QString &name, const QString &password)
{
    ptrUrl->setHost(host);
    ptrUrl->setPort(prot);
    ptrUrl->setUserName(name);
    ptrUrl->setPassword(password);
}

bool ftpSession::down(const QString &localFile, const QString &remoteFile)
{
    if(localFile.isNull() || remoteFile.isNull()) return false;
    if(ptrSession == nullptr){
        throw std::runtime_error("ptr is null");
    }
    curl_easy_reset(ptrSession);
    ptrUrl->setPath(remoteFile);
    QFile file(localFile);
    if(file.open(QFile::WriteOnly)){
//        char *t = ptrUrl->toString().toStdString().c_str();
        curl_easy_setopt(ptrSession,CURLOPT_URL,ptrUrl->toString().toStdString().c_str());
        curl_easy_setopt(ptrSession,CURLOPT_WRITEFUNCTION,&ftpSession::WriteToFileCallbask);
        curl_easy_setopt(ptrSession,CURLOPT_WRITEDATA,&file);
    }
    CURLcode err =perfrom();
    if(err != CURLE_OK){
        throw std::runtime_error("not down file");
        return false;
    }else {
        file.close();
        return true;
    }
}

size_t ftpSession::WriteToFileCallbask(void *buff, size_t size, size_t nmemb, void *data)
{
    if ((size == 0) || (nmemb == 0) || ((size * nmemb) < 1) || (data == nullptr)) return 0;
    QFile *file = reinterpret_cast<QFile *>(data);
    if(file->isOpen()){
        file->write(reinterpret_cast<char *>(buff),size * nmemb);
    }
    return size * nmemb;
}

CURLcode ftpSession::perfrom() const
{
    CURLcode err = CURLE_OK;
    err = curl_easy_perform(ptrSession);
    return err;
}
FTPSPACE_E
