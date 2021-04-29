/*
 * @Author: your name
 * @Date: 2020-11-24 17:36:51
 * @LastEditTime: 2020-11-29 15:53:34
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\test\test_qtCore.cpp
 */

#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

TEST(qtcore, settinf_key) {
//  auto uePath_key =
//      QSettings{R"(HKEY_LOCAL_MACHINE\SOFTWARE\EpicGames\Unreal Engine)",
//                QSettings::NativeFormat};
//  for (const auto &item : uePath_key.childGroups()) {
//    std::cout << item.toLocal8Bit().data() << "\n";
//    auto setting_ue = QSettings(
//        QString(R"(HKEY_LOCAL_MACHINE\SOFTWARE\EpicGames\Unreal Engine\%1)")
//            .arg(item),
//        QSettings::NativeFormat);
//    auto path = setting_ue.value("InstalledDirectory");
//    std::cout << path.toString().toStdString() << "\n";
//  }
}

TEST(qtcore, tounicode) {
//  const auto data = QString("中");
//  std::cout << data.unicode() << std::endl;
//  std::cout << data.at(0).unicode() << std::endl;
//  std::cout << data.size() << std::endl;
//  std::cout << QString::number(data.at(0).unicode(), 10).toInt();
}