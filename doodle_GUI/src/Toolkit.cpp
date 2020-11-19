#include <src/Toolkit.h>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/process.hpp>
#include <core_doQt.h>
#include <Logger.h>
#include <QtCore/QDir>
#include <QtGui/qclipboard.h>
#include <QtGui/QGuiApplication>
#include <QtWidgets/qmessagebox.h>
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
DOODLE_NAMESPACE_E


