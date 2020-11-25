#include <src/toolkit.h>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/process.hpp>
#include <core_doQt.h>
#include <Logger.h>
#include <QtCore/QDir>
#include <QtGui/qclipboard.h>
#include <QtGui/QGuiApplication>
#include <QtWidgets/qmessagebox.h>
#include <QtWidgets/qinputdialog.h>
#include <QtCore/qsettings.h>
#include <string>

DOODLE_NAMESPACE_S

void toolkit::openPath(const doCore::fileSqlInfoPtr& info_ptr,
                       const bool& openEx) {
  auto path = doCore::coreSet::getSet().getPrjectRoot() /
              info_ptr->getFileList()[0].parent_path();

  boost::format str("explorer.exe \"%s\"");
  auto path_noral =
      boost::replace_all_copy(path.generic_path().generic_string(), "/", "\\");
  path_noral = boost::replace_all_copy(path_noral, "\\\\", "\\");

  str % path_noral;

  DOODLE_LOG_INFO << "打开路径: " << str.str().c_str();
  if (boost::filesystem::exists(path)) {
    if (openEx)
      boost::process::system(str.str().c_str());
    else
      QGuiApplication::clipboard()->setText(DOTOS(path.generic_string()));
  } else {
    DOODLE_LOG_CRIT << QString::fromUtf8("没有在服务器中找到目录:\n %1")
                           .arg(DOTOS(path.generic_string()));
    QMessageBox::warning(nullptr, QString::fromUtf8("没有目录"),
                         QString::fromUtf8("没有在服务器中找到目录:\n %1")
                             .arg(DOTOS(path.generic_string())),
        QMessageBox::Yes);
  }
}
void toolkit::installUePath() {
  auto uePath_key = QSettings{R"(HKEY_LOCAL_MACHINE\SOFTWARE\EpicGames\Unreal Engine)",
                              QSettings::NativeFormat};
  doCore::dpathList dpath_list;
  for (const auto &item : uePath_key.childGroups()) {
    auto setting_ue = QSettings(QString(R"(HKEY_LOCAL_MACHINE\SOFTWARE\EpicGames\Unreal Engine\%1)")
                                    .arg(item),QSettings::NativeFormat);
    auto path = setting_ue.value("InstalledDirectory");
    dpath_list.push_back(path.toString().toStdString());
  }
  dpath_list.erase(std::remove_if(dpath_list.begin(),dpath_list.end(),[=](doCore::dpath &dpath){
    return boost::filesystem::exists(dpath);
  }) ,dpath_list.end());

  if (dpath_list.empty()){
    QMessageBox::warning(nullptr,QString::fromUtf8("注意"),QString::fromUtf8("没有在注册表中找到目录"));
    return;
  }

  QStringList list;
  for (const auto &item : dpath_list) {
    list.push_back(DOTOS(item.generic_string()));
  }
  auto ue_path = QInputDialog::getItem(nullptr,"选择安装ue插件的版本","路径",list);
  if (ue_path.isNull() || ue_path.isEmpty()) return;



}
DOODLE_NAMESPACE_E


