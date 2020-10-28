//
// Created by teXiao on 2020/10/26.
//

#include "mayaArchiveShotFbx.h"

#include "shotfilesqlinfo.h"
#include "fileclass.h"
#include "filetype.h"
#include "mayaArchive.h"


#include "episodes.h"
#include "shot.h"
#include "Logger.h"

#include <QTemporaryFile>
#include <QDir>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

CORE_NAMESPACE_S

mayaArchiveShotFbx::mayaArchiveShotFbx(shotInfoPtr &shot_info_ptr)
    : fileArchive(),
      p_info_ptr_(shot_info_ptr),
      p_maya_archive_ptr_(),
      p_temporary_file_() {
  p_maya_archive_ptr_ = std::make_shared<mayaArchive>(shot_info_ptr);
}
void mayaArchiveShotFbx::_generateFilePath() {
  if (!p_soureFile.empty())
    for (auto &k_i : p_soureFile)
      p_Path.push_back(p_info_ptr_->generatePath("export_fbx") + "/" + QFileInfo(k_i).fileName());
  else if (!p_info_ptr_->getFileList().empty())
    for (auto &&item: p_info_ptr_->getFileList())
      p_Path.push_back(item.filePath());
}
bool mayaArchiveShotFbx::exportFbx() {
  auto info = QFileInfo(p_maya_archive_ptr_->down().front());
  p_temporary_file_ = std::make_shared<QTemporaryFile>();
  p_temporary_file_->setFileTemplate(QDir::tempPath() + "/mayaExport_XXXXXX.py");
  //复制出导出脚本
  QFile file_tmp(":/resource/mayaExport.py");
  p_temporary_file_->open();
  if (file_tmp.open(QIODevice::ReadOnly))
    p_temporary_file_->write(file_tmp.readAll());
  p_temporary_file_->close();

  const auto mayapath = QString(R"("C:\Program Files\Autodesk\Maya2018\bin\mayapy.exe")");
  auto filePath = info.filePath();
  DOODLE_LOG_INFO << "导出文件" << filePath;

  auto comm = QString("%1 %2 --path %3 --name %4 --version %5 --suffix %6 --exportpath %7")
      .arg(mayapath)//maya py 解释器位置 -->1
      .arg(p_temporary_file_->fileName())//导出脚本位置           -->2
      .arg(info.path())//导出到文件的位置中--3
      .arg(info.baseName())//导出的名称  --4
      .arg(p_info_ptr_->getVersionP())//版本 --5
      .arg("." + info.suffix())//文件后缀 -- 6
      .arg(info.path());
  DOODLE_LOG_INFO << "导出命令" << comm;
//  auto popen = QProcess();
//  auto list = QStringList();
//  list << p_temporary_file_->fileName()
//  <<" --path "<< info.path()
//  <<" --name "<< info.baseName()
//  <<" --version "<< QString(p_info_ptr_->getVersionP())
//  <<" --suffix "<< "." + info.suffix()
//  <<" --exportpath "<< info.path();
//
//  popen.start(mayapath,list);
//  popen.waitForFinished();

  std::system(comm.toStdString().c_str());
  bool kJson = readExportJson(info.path());
  if (!kJson)
    p_state_ = state::fail;
  return kJson;
}
bool mayaArchiveShotFbx::readExportJson(const QString &exportPath) {
  auto k_s_file = QDir::cleanPath(exportPath) + "/doodle_Export.json";
  //读取文件
  QFile k_file(k_s_file);
  if (!k_file.open(QIODevice::ReadOnly)) return false;
  auto k_exjson = QJsonDocument::fromJson(k_file.readAll());

  if (k_exjson.isEmpty()) return false;
  if (!k_exjson.isObject()) return false;

  auto list = k_exjson.object();
  try {
    for (auto &&item :list)
      p_soureFile.push_back(item.toArray()[0].toString());
  }
  catch (...) {
    DOODLE_LOG_WARN << "获得导出文件失败, 导出maya失败" << k_s_file;
    return false;
  }
  return true;
}
bool mayaArchiveShotFbx::update() {
  bool var = true;
  var &= exportFbx();
  _generateFilePath();
  p_cacheFilePath = p_soureFile;
  _updata(p_soureFile);
  insertDB();
  p_state_ = state::success;
  return var;
}
void mayaArchiveShotFbx::insertDB() {
  auto shotInfo = std::make_shared<shotFileSqlInfo>();
  shotInfo->setFileList(p_Path);

  //获得导出类别的约束
  auto fileTypelist = fileType::getAll(p_info_ptr_->getFileclass());
  fileTypePtr k_fileType = nullptr;
  for (auto &&item:fileTypelist) {
    if (item->getFileType() == "export_fbx") {
      k_fileType = item;
      break;
    }
  }
  if (!k_fileType) {
    k_fileType.reset(new fileType());
    k_fileType->setFileType("export_fbx");
    k_fileType->setFileClass(p_info_ptr_->getFileclass());
    k_fileType->insert();
  }

  shotInfo->setFileType(k_fileType);
  shotInfo->setVersionP(p_info_ptr_->getVersionP());

  shotInfo->insert();

}
std::map<QString, QString> mayaArchiveShotFbx::getInfo() {
  auto map = std::map<QString, QString>();
  map.insert(std::make_pair("episodes",QString::number(p_info_ptr_->getEpisdes()->getEpisdes())));
  map.insert(std::make_pair("shot",QString::number(p_info_ptr_->getShot()->getShot())));
  return map;
}

CORE_NAMESPACE_E