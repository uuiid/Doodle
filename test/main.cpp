#include "t_fileSyn/test_filesyn.h"
#include "t_ftp/test_ftp.h"

#include "ftp_global.h"
#include "src/ftphandle.h"
#include <QFileInfo>
#include <QTest>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc,argv);
    app.setAttribute(Qt::AA_Use96Dpi,true);
    test_fileSyn tc1;
    QTEST_SET_MAIN_SOURCE_PATH
    QTest::qExec(&tc1,argc,argv);

    test_ftp tftp;
    QTEST_SET_MAIN_SOURCE_PATH
    QTest::qExec(&tftp,argc,argv);

    doFtp::ftpSessionPtr session = doFtp::ftphandle::getFTP().session("192.168.10.213",
                                                                   21,"dubuxiaoyaozhangyubin","zhangyubin");
//    session->down("D:/tmp/test.exe","/dist/doodle.exe");
    session->updata("D:/tmp/BuJu.1001.png","/cache/test.png");
    QObject::connect(&(*session),&doFtp::ftpSession::finished,
                     &app,   &QCoreApplication::quit);

    app.exec();
    return 0;
}

