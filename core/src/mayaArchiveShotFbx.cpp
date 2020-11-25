//
// Created by teXiao on 2020/10/26.
//

#include "mayaArchiveShotFbx.h"
#include <src/shotfilesqlinfo.h>
#include <src/shotClass.h>
#include <src/shottype.h>

#include <src/episodes.h>
#include <src/shot.h>
#include <Logger.h>

#include <src/coreset.h>
#include <stdexcept>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <nlohmann/json.hpp>
#include <boost/process.hpp>
#include <sstream>
#include <filesystem>
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
      p_Path.push_back(
          p_info_ptr_->generatePath("export_fbx")
              / k_i.filename()
      );
  else if (!p_info_ptr_->getFileList().empty())
    for (auto &&item: p_info_ptr_->getFileList())
      p_Path.push_back(item.string());
}
bool mayaArchiveShotFbx::exportFbx(const dpath &shot_data) {
  auto resou = boost::filesystem::current_path().parent_path() / "resource";

  //复制出导出脚本
  boost::filesystem::copy(resou / "mayaExport.py", *p_temporary_file_);

  const auto mayapath = dstring{"mayapy.exe"};
  DOODLE_LOG_INFO << "导出文件" << shot_data.string().c_str();

  boost::format str("%1% %2% --path %3% --name %4% --version %5% --suffix %6% --exportpath %7%");
  str % mayapath
      % p_temporary_file_->generic_string()
      % shot_data.parent_path().generic_string()
      % boost::filesystem::basename(shot_data)
      % 0
      % boost::filesystem::extension(shot_data)
      % shot_data.parent_path().generic_string();

  DOODLE_LOG_INFO << "导出命令" << str.str().c_str();

  auto env = boost::this_process::environment();

  if (boost::filesystem::exists(R"(C:\Program Files\Autodesk\Maya2018\bin)")) {
    env["PATH"] += R"(C:\Program Files\Autodesk\Maya2018\bin\)";
  } else if (boost::filesystem::exists(R"(C:\Program Files\Autodesk\Maya2019\bin)")) {
    env["PATH"] += R"(C:\Program Files\Autodesk\Maya2019\bin\)";
  } else if (boost::filesystem::exists(R"(C:\Program Files\Autodesk\Maya2020\bin)")) {
    env["PATH"] += R"(C:\Program Files\Autodesk\Maya2020\bin\)";
  } else {
    return false;
  }

  boost::process::system(str.str(), env);

  return boost::filesystem::exists(shot_data.parent_path() / "doodle_Export.json");
}
bool mayaArchiveShotFbx::readExportJson(const dpath &exportPath) {
  auto k_s_file = exportPath / "doodle_Export.json";
  //读取文件
  boost::filesystem::ifstream kIfstream{};
  kIfstream.open(k_s_file);
  std::stringstream kStringstream;
  kStringstream << kIfstream.rdbuf();
  kIfstream.close();

  bool re = true;
  try {
    DOODLE_LOG_INFO << kStringstream.str().c_str();
    nlohmann::json root = nlohmann::json::parse(kStringstream.str());
    for (auto &item:root.items()) {
      auto str = item.value()[0].get<dstring>();
      p_soureFile.push_back(str);
    }
    re = true;
  } catch (nlohmann::json::parse_error &err) {
    DOODLE_LOG_WARN << "not export maya fbx" << err.what();
    re = false;
  }

  return re;
}
bool mayaArchiveShotFbx::update(const dpath &shot_data) {
  if (shot_data.empty()) return false;
  p_info_ptr_->setShotType(shotType::findShotType("maya_export"));
  //获得缓存路径并下载文件
  auto cache_path = p_info_ptr_->generatePath("export_fbx");
  cache_path = coreSet::getSet().getCacheRoot() / cache_path;
  p_Path = {shot_data};
  _down({cache_path});

  //确认导出成功
  //if (!exportFbx(cache_path / shot_data.filename()))
  //  return false;
  //读取导出文件的设置并进行确认
  //并设置文件来源
  if (!readExportJson(cache_path)) {
    p_state_ = state::fail;
    return false;
  }
  p_Path.clear();
  p_cacheFilePath.clear();
  //开始上传文件
  _generateFilePath();
  p_cacheFilePath = p_soureFile;
  _updata(p_soureFile);
  insertDB();
  p_state_ = state::success;
  return true;
}
void mayaArchiveShotFbx::insertDB() {
  p_info_ptr_->setFileList(p_Path);

  p_info_ptr_->setShotType(doCore::shotType::findShotType("maya_export", true));
  p_info_ptr_->setInfoP("导出fbx文件");
  p_info_ptr_->insert();

}

CORE_NAMESPACE_E