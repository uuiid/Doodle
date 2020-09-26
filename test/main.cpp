#include <iostream>

// #include "t_fileSyn/test_filesyn.h"
// #include "t_ftp/test_ftp.h"
// #include "t_convert/test_convert.h"
#include <QCoreApplication>
#include <QTextCodec>

#include <gtest/gtest.h>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);


    QTextCodec *codec = QTextCodec::codecForName("GBK");
    QTextCodec::setCodecForLocale(codec);
    
    ::testing::InitGoogleTest(&argc, argv);
    
    RUN_ALL_TESTS();

    return 0;
}
