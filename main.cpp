#include "fileSyn_global.h"
#include "src/sqlconnect.h"

#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    doFileSyn::sqlConnect& taa = doFileSyn::sqlConnect::GetSqlConnect();
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
