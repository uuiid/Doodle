#include "test_ftp.h"
#include "ftp_global.h"
#include "src/ftphandle.h"
#include <QFileInfo>
#include <QTest>
#include <QDateTime>
test_ftp::test_ftp()
{

}

void test_ftp::test_upload()
{
    doFtp::ftpSessionPtr session = doFtp::ftphandle::getFTP().session("192.168.10.213",
                                                                      21,
                                                                      "dubuxiaoyaozhangyubin",
                                                                      "zhangyubin");
    QVERIFY(session->upload("D:/tmp/BuJu.1002.png","/cache/test/test.png"));
}

void test_ftp::test_down()
{
    doFtp::ftpSessionPtr session = doFtp::ftphandle::getFTP().session("192.168.10.213",
                                                                      21,"","");
    QVERIFY(session->down("D:/tmp/test.exe","/dist/doodle.exe"));
}

void test_ftp::test_getInfo()
{
    doFtp::ftpSessionPtr session = doFtp::ftphandle::getFTP().session("192.168.10.213",
                                                                      21,"","");
    doFtp::oFileInfo info = session->fileInfo("/dist/doodle.exe");
    qDebug() <<QDateTime::fromTime_t(info.fileMtime);
    qDebug() <<(info.fileSize)/(1024 * 1024) << "/mb";
}

void test_ftp::test_getList()
{
    doFtp::ftpSessionPtr session = doFtp::ftphandle::getFTP().session("192.168.10.213",
                                                                      21,"","");
    std::vector<doFtp::oFileInfo> info = session->list("/dist/");
    for(unsigned int i = 0;i <info.size();++i){
        qDebug() << "is folder" << info[i].isFolder;
        qDebug() << "path :" << info[i].filepath;
    }
}
