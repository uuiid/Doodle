//
// Created by teXiao on 2020/11/24.
//
#include <gtest/gtest.h>
#include <QtCore/qsettings.h>
#include <boost/filesystem.hpp>
TEST(qtcore,settinf_key){
  auto uePath_key = QSettings{R"(HKEY_LOCAL_MACHINE\SOFTWARE\EpicGames\Unreal Engine)",
                              QSettings::NativeFormat};
  for (const auto &item : uePath_key.childGroups()) {
    std::cout << item.toLocal8Bit().data() << "\n";
    auto setting_ue = QSettings(QString(R"(HKEY_LOCAL_MACHINE\SOFTWARE\EpicGames\Unreal Engine\%1)")
        .arg(item),QSettings::NativeFormat);
    auto path = setting_ue.value("InstalledDirectory");
    std::cout << path.toString().toStdString() << "\n";

  }

}
