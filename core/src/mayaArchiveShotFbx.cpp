//
// Created by teXiao on 2020/10/26.
//

#include "mayaArchiveShotFbx.h"
#include <src/shotfilesqlinfo.h>
#include <src/shotClass.h>
#include <src/shottype.h>
#include <src/mayaArchive.h>

#include <src/episodes.h>
#include <src/shot.h>
#include <Logger.h>

#include <QDir>

#include <stdexcept>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <json/json.h>
#include <boost/process.hpp>
#include <ObjIdlbase.h>
CORE_NAMESPACE_S

mayaArchiveShotFbx::mayaArchiveShotFbx(shotInfoPtr &shot_info_ptr)
    : fileArchive(),
      p_info_ptr_(shot_info_ptr),
      p_temporary_file_(std::make_shared<dpath>(boost::filesystem::temp_directory_path())) {
  *p_temporary_file_ = *p_temporary_file_ / boost::filesystem::unique_path();
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
  auto info = kArchivePtr->down();

  auto resou = boost::filesystem::current_path().parent_path() / "resource";

  //复制出导出脚本
  boost::filesystem::copy(resou / "mayaExport.py",*p_temporary_file_);


  const auto mayapath = dstring{"mayapy.exe"};
  DOODLE_LOG_INFO << "导出文件" << info.string().c_str();

  boost::format str("%1% %2% --path %3% --name %4% --version %5% --suffix %6% --exportpath %7%");
  str % mayapath
      % p_temporary_file_->generic_string()
      % info.parent_path().generic_string()
      % boost::filesystem::basename(info)
      % shot_data->getVersionP()
      % boost::filesystem::extension(info)
      % info.parent_path();

  DOODLE_LOG_INFO << "导出命令" << str.str().c_str();
  auto env = boost::this_process::environment();
  env["PATH"] += R"(C:\Program Files\Autodesk\Maya2018\bin\)";

  boost::process::system(str.str(), env);
  bool kJson = readExportJson(info.parent_path());
  if (!kJson) {
    p_state_ = state::fail;
  }
  p_info_ptr_->setVersionP(shot_data->getVersionP());
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
    return false;
  }
  for(auto &&item:root){
    p_soureFile.push_back(item[0].asString());
  }
  return true;
}
bool mayaArchiveShotFbx::update(shotInfoPtr &shot_data) {
  if (!exportFbx(shot_data))
    return false;
  _generateFilePath();
  p_cacheFilePath = p_soureFile;
  _updata(p_soureFile);
  insertDB();
  p_state_ = state::success;
  return true;
}
void mayaArchiveShotFbx::insertDB() {
  p_info_ptr_->setFileList(p_Path);

  p_info_ptr_->setShotType(doCore::shotType::findShotType("export_fbx",true));

  p_info_ptr_->insert();

}
std::map<QString, QString> mayaArchiveShotFbx::getInfo() {
  auto map = std::map<QString, QString>();
  map.insert(std::make_pair("episodes", QString::number(p_info_ptr_->getEpisdes()->getEpisdes())));
  map.insert(std::make_pair("shot", QString::number(p_info_ptr_->getShot()->getShot())));
  return map;
}

CORE_NAMESPACE_E