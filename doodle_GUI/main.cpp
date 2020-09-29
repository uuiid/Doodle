#include "src/coreset.h"

//测试时导入
#include "src/mainWindows.h"
//必要导入
#include <QApplication>
#include <QTextCodec>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //设置本地编码
    QTextCodec *codec = QTextCodec::codecForName("GBK");
    QTextCodec::setCodecForLocale(codec);

    doCore::coreSet &set = doCore::coreSet::getCoreSet();
    set.init();

    doodle::mainWindows main_doodle = doodle::mainWindows();
    main_doodle.show();
    
    return a.exec();
}