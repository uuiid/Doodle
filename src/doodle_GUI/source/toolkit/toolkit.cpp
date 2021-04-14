#include <doodle_GUI/source/toolkit/toolkit.h>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include <corelib/core_Cpp.h>

#include <loggerlib/Logger.h>

#include <boost/format.hpp>

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

DOODLE_NAMESPACE_E
