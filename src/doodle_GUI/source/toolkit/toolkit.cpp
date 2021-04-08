﻿#include <doodle_GUI/source/toolkit/toolkit.h>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/process.hpp>
#include <corelib/core_Cpp.h>
#include <loggerlib/Logger.h>
#include <QtCore/QDir>
#include <QtGui/qclipboard.h>
#include <QtGui/QGuiApplication>
#include <QtWidgets/qmessagebox.h>
#include <QtWidgets/qinputdialog.h>

#include <QtCore/qsettings.h>

#include <boost/format.hpp>
#include <boost/process.hpp>
#include <loggerlib/Logger.h>
#include <string>
#include <regex>
DOODLE_NAMESPACE_S

// void toolkit::openPath(const fileSqlInfoPtr &info_ptr,
//                        const bool &openEx) {
//   if (info_ptr->getFileList().empty()) {
//     DOODLE_LOG_INFO("没有找到目录");
//     QMessageBox::warning(nullptr, QString::fromUtf8("没有目录"),
//                          QString::fromUtf8("没有找到目录 "),
//                          QMessageBox::Yes);
//   }
//   auto path =
//       info_ptr->getFileList()[0].parent_path();
//   boost::wformat wstr;
//   boost::format str("explorer.exe \"%s\"");
//   auto path_noral = boost::replace_all_copy(path.generic_path().generic_string(), "/", "\\");
//   path_noral      = boost::replace_all_copy(path_noral, R"(\\)", R"(\)");
//   path_noral      = boost::replace_all_copy(path_noral, R"(\\)", R"(\)");
//   str % path_noral;

//   DOODLE_LOG_INFO("打开路径: " << str.str().c_str());
//   if (boost::filesystem::exists(path)) {
//     if (openEx)
//       boost::process::system(str.str().c_str());
//     else
//       QGuiApplication::clipboard()->setText(QString::fromStdString(path.generic_string()));
//   } else {
//     DOODLE_LOG_INFO(" 没有在服务器中找到目录:" << path.generic_string());
//     QMessageBox::warning(nullptr, QString::fromUtf8("没有目录"),
//                          QString::fromUtf8("没有在服务器中找到目录:\n %1")
//                              .arg(QString::fromStdString(path.generic_string())),
//                          QMessageBox::Yes);
//   }
// }

void toolkit::openPath(const fileSys::path &path_) {
  auto path = path_;

  boost::format str("explorer.exe \"%s\"");
  auto path_noral =
      boost::replace_all_copy(path.generic_path().generic_string(), "/", "\\");
  path_noral = boost::replace_all_copy(path_noral, R"(\\)", R"(\)");
  path_noral = boost::replace_all_copy(path_noral, R"(\\)", R"(\)");

  str % path_noral;

  DOODLE_LOG_INFO("打开路径: " << str.str().c_str());
  if (boost::filesystem::exists(path)) {
    boost::process::system(str.str().c_str());
  } else {
    DOODLE_LOG_INFO("没有在服务器中找到目录:" << path.generic_string());
    QMessageBox::warning(nullptr, QString::fromUtf8("没有目录"),
                         QString::fromUtf8("没有在服务器中找到目录:\n %1")
                             .arg(QString::fromStdString(path.generic_string())),
                         QMessageBox::Yes);
  }
}
void toolkit::installUePath(const std::string &path) {
  boost::filesystem::path ue_path;
  boost::filesystem::path sourePath;
  if (path.empty()) {
    ue_path = getUeInstallPath();
    if (ue_path.empty()) return;

    //判断并选择来源
    if (std::regex_search(ue_path.generic_string(), std::regex{"4.25"})) {
      sourePath = coreSet::getSet().program_location().parent_path() /
                  "plug/uePlug/4.25/Plugins/Doodle";
    } else if (std::regex_search(ue_path.generic_string(),
                                 std::regex{"4.26"})) {
      sourePath = coreSet::getSet().program_location().parent_path() /
                  "plug/uePlug/4.26/Plugins/Doodle";
    } else {
      QMessageBox::warning(nullptr, QString::fromUtf8("注意"),
                           QString::fromUtf8("没有编译这个版本的插件"));
      return;
    }
    ue_path = ue_path / R"(Engine\Plugins\Doodle)";
  } else {
    //获得选择目标
    QStringList list;
    list << QString::number(4.25) << QString::number(4.26);
    auto ue_version =
        QInputDialog::getItem(nullptr, "选择安装ue插件的版本", "路径", list)
            .toFloat();
    if (ue_version == 4.25) {
      sourePath = coreSet::getSet().program_location().parent_path() /
                  "plug/uePlug/4.25/Plugins/Doodle";
    } else {
      sourePath = coreSet::getSet().program_location().parent_path() /
                  "plug/uePlug/4.26/Plugins/Doodle";
    }
    ue_path = boost::filesystem::path{path}.parent_path() / "Plugins/Doodle";
  }

  if (boost::filesystem::exists(ue_path)) {
    boost::filesystem::remove_all(ue_path);
  }
  FileSystem::localCopy(sourePath, ue_path, false);
}

void toolkit::modifyUeCachePath() {
  //"%ENGINEVERSIONAGNOSTICUSERDIR%DerivedDataCache"
  //替换为 "%GAMEDIR%DerivedDataCache"
  auto ue_path = getUeInstallPath();
  if (ue_path.empty()) return;

  ue_path = ue_path / "Engine/Config/BaseEngine.ini";
  //做备份
  FSys::copy(ue_path, FSys::path{ue_path}.replace_extension(".ini.backup"), FSys::copy_options::update_existing);

  FSys::fstream file{ue_path, std::ios::in | std::ios::out | std::ios::binary};
  std::string line{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};

  static std::string str{R"("%ENGINEVERSIONAGNOSTICUSERDIR%DerivedDataCache")"};
  auto it = line.find(str);
  while (it != std::string::npos) {
    line.replace(it, str.size(), R"("%GAMEDIR%DerivedDataCache")");
    it = line.find(str);
  }
  file.close();
  file.open(ue_path, std::ios::out | std::ios::trunc | std::ios::binary);
  file << line;

  QMessageBox::information(nullptr, ("信息"), ("完成修改 \n备份旧文件为 BaseEngine.ini.backup"));
}

bool toolkit::update() {
  return false;
}

bool toolkit::deleteUeCache() {
  boost::format str{R"(%1%\UnrealEngine)"};
  char *k_path;
  size_t k_path_len;
  getenv_s(&k_path_len, NULL, 0, "LOCALAPPDATA");
  if (k_path_len == 0) return false;
  k_path = (char *)malloc(k_path_len * sizeof(char));
  if (!k_path) return false;

  getenv_s(&k_path_len, k_path, k_path_len, "LOCALAPPDATA");
  std::string k_str(k_path);
  str % k_str;
  free(k_path);

  try {
    DOODLE_LOG_INFO("delete Folder : " << str.str());
    boost::filesystem::remove_all(str.str());
    QMessageBox::information(nullptr, ("信息"), ("完成缓存清除"));
    return true;

  } catch (const std::exception &e) {
    DOODLE_LOG_ERROR(e.what());
    QMessageBox::information(nullptr, ("信息"), ("缓存清除失败"));
    return false;
  }
}

fileSys::path toolkit::getUeInstallPath() {
  boost::filesystem::path ue_path;
  boost::filesystem::path sourePath;

  auto uePath_key =
      QSettings{R"(HKEY_LOCAL_MACHINE\SOFTWARE\EpicGames\Unreal Engine)",
                QSettings::NativeFormat};
  dpathList dpath_list;
  for (const auto &item : uePath_key.childGroups()) {
    auto setting_ue = QSettings(
        QString(R"(HKEY_LOCAL_MACHINE\SOFTWARE\EpicGames\Unreal Engine\%1)")
            .arg(item),
        QSettings::NativeFormat);
    auto kPath = setting_ue.value("InstalledDirectory");
    dpath_list.push_back(kPath.toString().toStdString());
  }
  dpath_list.erase(std::remove_if(dpath_list.begin(), dpath_list.end(),
                                  [=](fileSys::path &dpath) {
                                    return !boost::filesystem::exists(dpath);
                                  }),
                   dpath_list.end());

  if (dpath_list.empty()) {
    QMessageBox::warning(nullptr, QString::fromUtf8("注意"),
                         QString::fromUtf8("没有在注册表中找到目录"));
    return {};
  }
  //获得选择目标
  QStringList list;
  for (const auto &item : dpath_list) {
    list.push_back(QString::fromStdString(item.generic_string()));
  }
  ue_path = QInputDialog::getItem(nullptr, "选择ue版本", "路径", list)
                .toStdString();
  if (ue_path.empty()) return {};

  if (!boost::filesystem::exists(ue_path)) {
    return {};
  }
  return ue_path;
}
DOODLE_NAMESPACE_E
