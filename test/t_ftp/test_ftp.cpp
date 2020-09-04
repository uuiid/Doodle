#include "test_ftp.h"
#include "ftp_global.h"
#include "src/ftphandle.h"
#include <QFileInfo>
#include <QTest>

test_ftp::test_ftp()
{

}

void test_ftp::test_d()
{
    doFtp::ftpSessionPtr session = doFtp::ftphandle::getFTP().session("192.168.10.213",
                                                                   21,"","");
    session->down("D:/tmp/test.exe","/dist/key.txt");
}

void test_ftp::test_down()
{
//    QUrl url("");
//    url.setScheme("ftp");
//    url.setHost("192.168.10.213");
//    url.setPort(21);
//    url.setPath("/dist/doodle.exe");
////    qDebug() << url.toString();
//    doFtp::ftphandle& ftp = doFtp::ftphandle::getFTP();
////    ftp.downfile(QUrl("ftp://192.168.10.213:21/dist/key.txt"),"D:/tmp/test.exe");
//    ftp.downfile(QUrl("https://www.baidu.com/"),"D:/tmp/test.exe");
}
