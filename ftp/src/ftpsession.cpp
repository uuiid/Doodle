#include "ftpsession.h"

#include <fstream>


FTPSPACE_S
ftpSession::ftpSession()
{
    ptrUrl = QSharedPointer<QUrl>(new QUrl());
    outfile = QSharedPointer<QFile>(new QFile());
    inputfile = QSharedPointer<QFile>(new QFile());

    curlSession = curl_easy_init();
}

ftpSession::~ftpSession()
{
    curl_easy_cleanup(curlSession);
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

bool ftpSession::down(const QString &localFile, const QString &remoteFile)
{
    //打开文件
    outfile->setFileName(localFile);
    if(!outfile->open(QIODevice::WriteOnly)){
        throw std::runtime_error("not open file");
        return false;
    }
    //设置url路径
    ptrUrl->setPath(remoteFile);
    curl_easy_reset(curlSession);

    //创建下载设置
    curl_easy_setopt(curlSession,CURLOPT_URL,ptrUrl->toString().toStdString().c_str());
    curl_easy_setopt(curlSession,CURLOPT_WRITEFUNCTION,&ftpSession::writeFileCallbask);
    curl_easy_setopt(curlSession,CURLOPT_WRITEDATA,this->outfile.get());

    //检查下载是否成功
    CURLcode err=perform();
    outfile->close();
    if(err != CURLE_OK){
        qDebug()<< err << curl_easy_strerror(err);
        outfile->remove();
        return false;
    }
    emit finished();
    return true;
}

bool ftpSession::upload(const QString &localFile, const QString &remoteFile)
{
    //打开本地文件准备上传
    inputfile->setFileName(localFile);
    if(!inputfile->open(QIODevice::ReadOnly)){
        throw std::runtime_error("not open file(Read Only)");
        return false;
    }
    ptrUrl->setPath(remoteFile);
    curl_easy_reset(curlSession);

    //创建上传设置
    curl_easy_setopt(curlSession,CURLOPT_URL,ptrUrl->toString().toStdString().c_str());
    curl_easy_setopt(curlSession,CURLOPT_UPLOAD,1L);
    curl_easy_setopt(curlSession,CURLOPT_FTP_CREATE_MISSING_DIRS,CURLFTP_CREATE_DIR_RETRY);
    curl_easy_setopt(curlSession,CURLOPT_INFILESIZE,inputfile->size());
    curl_easy_setopt(curlSession,CURLOPT_READFUNCTION, &ftpSession::readFileCallbask);
    curl_easy_setopt(curlSession,CURLOPT_READDATA,inputfile.get());

    CURLcode err = perform();
    inputfile->close();
    if(err != CURLE_OK){
        qDebug()<<err<<curl_easy_strerror(err);
        return false;
    }
    emit finished();
    return true;
}

oFileInfo ftpSession::fileInfo(const QString &remoteFile)
{
    if(remoteFile.isEmpty()) throw std::runtime_error("remote file is NULL");
    ptrUrl->setPath(remoteFile);

    curl_easy_reset(curlSession);

    oFileInfo info;

    curl_easy_setopt(curlSession,CURLOPT_URL, ptrUrl->toString().toStdString().c_str());
    curl_easy_setopt(curlSession,CURLOPT_NOBODY,1L);//不获取文件本身
    curl_easy_setopt(curlSession,CURLOPT_FILETIME,1L);//获得文件时间
    curl_easy_setopt(curlSession,CURLOPT_HEADERFUNCTION,&ftpSession::notCallbask);
    curl_easy_setopt(curlSession,CURLOPT_HEADER,0L);

    CURLcode err = perform();

    if(err == CURLE_OK){
        long ftime = -1;
        err = curl_easy_getinfo(curlSession,CURLINFO_FILETIME, &ftime);
        if(err == CURLE_OK && ftime >0){
            info.fileMtime = static_cast<time_t>(ftime);
        }
        err = curl_easy_getinfo(curlSession,CURLINFO_CONTENT_LENGTH_DOWNLOAD,&info.fileSize);
    }else {
        info.fileMtime = -1;
        info.fileSize = 0;
    }
    info.filepath = remoteFile;
    info.isFolder = false;
    return info;
}
/*
获得服务器上的文件列表
*/
std::vector<oFileInfo> ftpSession::list(const QString &remoteFolder)
{
    const QStringList remote = remoteFolder.split("/",QString::SkipEmptyParts);
    ptrUrl->setPath(remoteFolder);

    curl_easy_reset(curlSession);//重置保护
    QString folderList;

    curl_easy_setopt(curlSession, CURLOPT_URL, ptrUrl->toString().toStdString().c_str());
    curl_easy_setopt(curlSession, CURLOPT_WRITEFUNCTION, &ftpSession::writeStringCallbask);
    curl_easy_setopt(curlSession, CURLOPT_WRITEDATA, &folderList);

    CURLcode err = perform();
    if(err != CURLE_OK) {
        throw std::runtime_error(curl_easy_strerror(err));
    }
    QStringList::const_iterator iter_folder;
    QStringList list_folder = folderList.split("\r\n",QString::SkipEmptyParts);
    std::vector<oFileInfo> listInfo;
    for(iter_folder = list_folder.begin();iter_folder != list_folder.end();iter_folder++){
        oFileInfo info;
        info.isFolder =false;
        QStringList folderInfo = iter_folder->split(" ",QString::SkipEmptyParts);
        if(folderInfo[0].at(0) == "d") info.isFolder = true;
        //folderInfo[folderInfo.size() - 1]
        QStringList p(remote);
        p.prepend("");
        p.append(folderInfo[folderInfo.size() - 1]);
        info.filepath = p.join("/");
        listInfo.push_back(info);
    }
    return listInfo;
}

size_t ftpSession::writeFileCallbask(void * buff,size_t size,size_t nmemb, void *data)
{
    if ((size == 0) || (nmemb == 0) || ((size * nmemb) < 1) || (data == nullptr)) return 0;
    QFile * file = static_cast<QFile *>(data);
    if(file->isOpen()){
        file->write(reinterpret_cast<char *>(buff), size *nmemb);
    }
    return size * nmemb;
}

size_t ftpSession::readFileCallbask(void *buff, size_t size, size_t nmemb, void *data)
{
    QFile *file = static_cast<QFile *>(data);
    return file->read(reinterpret_cast<char *>(buff),size * nmemb);
}

size_t ftpSession::notCallbask(void *buff, size_t size, size_t nmemb, void *data)
{
    return size * nmemb;
}

size_t ftpSession::writeStringCallbask(void *ptr, size_t size, size_t nmemb, void *data)
{
    if ((size == 0) || (nmemb == 0) || ((size * nmemb) < 1) || (data == nullptr)) return 0;
    QString * ptrlist = static_cast<QString *>(data);
    if(ptrlist != nullptr){
        ptrlist->append(QByteArray(reinterpret_cast<char *>(ptr),size * nmemb));
        return size * nmemb;
    }
    return 0;
}


CURLcode ftpSession::perform()
{
    CURLcode err = CURLE_OK;
    err = curl_easy_perform(curlSession);
    return err;
}
FTPSPACE_E
