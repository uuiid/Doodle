//
// Created by teXiao on 2020/10/26.
//

#include "mayaArchiveShotFbx.h"

#include "shotfilesqlinfo.h"
#include "shotClass.h"
#include "shottype.h"
#include "mayaArchive.h"

#include "episodes.h"
#include "shot.h"
#include "Logger.h"

#include <QDir>

#include <stdexcept>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <json/json.h>
CORE_NAMESPACE_S

mayaArchiveShotFbx::mayaArchiveShotFbx(shotInfoPtr &shot_info_ptr)
    : fileArchive(),
      p_info_ptr_(shot_info_ptr),
      p_temporary_file_(std::make_shared<dpath>(boost::filesystem::unique_path())) {

}
void mayaArchiveShotFbx::_generateFilePath() {
  if (!p_soureFile.empty())
    for (auto &k_i : p_soureFile)
      p_Path.push_back((p_info_ptr_->generatePath("export_fbx")
          / boost::filesystem::extension(k_i)).string());
  else if (!p_info_ptr_->getFileList().empty())
    for (auto &&item: p_info_ptr_->getFileList())
      p_Path.push_back(item.string());
}
bool mayaArchiveShotFbx::exportFbx(shotInfoPtr &shot_data) {
  if (!shot_data) {
    DOODLE_LOG_WARN << "没有数据传入";
    throw std::runtime_error("没有数据传入");
  }
  auto kArchivePtr = std::make_shared<mayaArchive>(shot_data);
  auto info = dpath (kArchivePtr->down().front());
//  p_temporary_file_ = std::make_shared<QTemporaryFile>();


//  p_temporary_file_->setFileTemplate(QDir::tempPath() + "/mayaExport_XXXXXX.py");
  //复制出导出脚本
  QFile file_tmp(":/resource/mayaExport.py");
  boost::filesystem::ofstream out;
  out.open(*p_temporary_file_,std::ofstream::out);
//  p_temporary_file_->open();
  if (file_tmp.open(QIODevice::ReadOnly))
    out<<file_tmp.readAll().toStdString();
  out.close();

  const auto mayapath = dstring (R"("C:\Program Files\Autodesk\Maya2018\bin\mayapy.exe")");
  DOODLE_LOG_INFO << "导出文件" << info.string().c_str();

  boost::format str("%1 %2 --path %3 --name %4 --version %5 --suffix %6 --exportpath %7");
  str % mayapath
      % p_temporary_file_->generic_string()
      % info.generic_string()
      % boost::filesystem::basename(info)
      % shot_data->getVersionP()
      % boost::filesystem::extension(info)
      % info.parent_path();
//  auto comm = QString("%1 %2 --path %3 --name %4 --version %5 --suffix %6 --exportpath %7")
//      .arg(mayapath)//maya py 解释器位置 -->1
//      .arg(p_temporary_file_->fileName())//导出脚本位置           -->2
//      .arg(info.path())//导出到文件的位置中--3
//      .arg(info.baseName())//导出的名称  --4
//      .arg(shot_data->getVersionP())//版本 --5
//      .arg("." + info.suffix())//文件后缀 -- 6
//      .arg(info.path());
  DOODLE_LOG_INFO << "导出命令" << str.str().c_str();
//  auto popen = QProcess();
//  auto list = QStringList();
//  list << p_temporary_file_->fileName()
//  <<" --path "<< info.path()
//  <<" --name "<< info.baseName()
//  <<" --version "<< QString(shot_data->getVersionP())
//  <<" --suffix "<< "." + info.suffix()
//  <<" --exportpath "<< info.path();
//
//  popen.start(mayapath,list);
//  popen.waitForFinished();
  std::system(str.str().c_str());
  bool kJson = readExportJson(info.parent_path());
  if (!kJson) {
    p_state_ = state::fail;
  }
  p_info_ptr_->setVersionP(shot_data->getVersionP());
  update();
  return kJson;
}
bool mayaArchiveShotFbx::readExportJson(const dpath &exportPath) {
  auto k_s_file = exportPath / "/doodle_Export.json";
  //读取文件
  boost::filesystem::ifstream rfile;
  rfile.open(k_s_file,std::ifstream::in);
  Json::CharReaderBuilder builder;
  Json::String err;
  Json::Value root;

  if(!Json::parseFromStream(builder,rfile,&root,&err)){
    DOODLE_LOG_WARN << err.c_str();
  }
  for(auto &&item:root){
    p_soureFile.push_back(item[0].asString());
  }
  return true;
}
bool mayaArchiveShotFbx::update() {
  _generateFilePath();
  p_cacheFilePath = p_soureFile;
  _updata(p_soureFile);
  insertDB();
  p_state_ = state::success;
  return true;
}
void mayaArchiveShotFbx::insertDB() {
  p_info_ptr_->setFileList(p_Path);

  p_info_ptr_->setShotType(p_info_ptr_->findFileType("export_fbx"));

  p_info_ptr_->insert();

}
std::map<QString, QString> mayaArchiveShotFbx::getInfo() {
  auto map = std::map<QString, QString>();
  map.insert(std::make_pair("episodes", QString::number(p_info_ptr_->getEpisdes()->getEpisdes())));
  map.insert(std::make_pair("shot", QString::number(p_info_ptr_->getShot()->getShot())));
  return map;
}
void mayaArchiveShotFbx::operator()(shotInfoPtr &shot_data) {
  exportFbx(shot_data);
  update();
}

CORE_NAMESPACE_E