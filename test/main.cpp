#include "t_fileSyn/test_filesyn.h"
#include "t_ftp/test_ftp.h"


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

    return 0;
}

