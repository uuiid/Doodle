/*
 * @Author: your name
 * @Date: 2020-09-27 14:33:50
 * @LastEditTime: 2020-12-01 13:58:59
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\fileArchive\mayaArchive.cpp
 */
#include "mayaArchive.h"
#include <src/core/coreset.h>
#include <src/shots/shotfilesqlinfo.h>

#include <Logger.h>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <boost/process.hpp>
#include <sstream>
#include <filesystem>
#include <stdexcept>
DOODLE_NAMESPACE_S
mayaArchive::mayaArchive(fileSqlInfoPtr shot_data)
    : p_info_ptr_(std::move(shot_data)) {}

bool mayaArchive::useUpdataCheck() const {
  return true;
}

bool mayaArchive::updataCheck() const {
  return true;
}

bool mayaArchive::useDownloadCheck() const {
  return true;
}

bool mayaArchive::downloadCheck() const {
  return true;
}

bool mayaArchive::CheckMaterialAndMapSet() const {
  auto resou = coreSet::program_location().parent_path() /
               "resource" / "asset" /
               "maya_model" / "chick_maya_run.py";
  // static const auto tmpFile = boost::filesystem::temp_directory_path();
  // auto runPath              = tmpFile / boost::filesystem::unique_path("%%%%_%%%%.py");
  // boost::filesystem::copy_file(resou, runPath);

  boost::format str("mayapy.exe %1% --path=%2% --exportpath=%3%");
  str % resou.generic_string()                                   //
      % p_cacheFilePath.front().generic_string()                 //
      % p_cacheFilePath.front().parent_path().generic_string();  //

  std::string mayaPY_path = "";
  if (boost::filesystem::exists(R"(C:\Program Files\Autodesk\Maya2018\bin)")) {
    mayaPY_path = R"(C:\Program Files\Autodesk\Maya2018\bin\)";
  } else if (boost::filesystem::exists(R"(C:\Program Files\Autodesk\Maya2019\bin)")) {
    mayaPY_path = R"(C:\Program Files\Autodesk\Maya2019\bin\)";
  } else if (boost::filesystem::exists(R"(C:\Program Files\Autodesk\Maya2020\bin)")) {
    mayaPY_path = R"(C:\Program Files\Autodesk\Maya2020\bin\)";
  } else {
    return false;
  }
  DOODLE_LOG_INFO("导出命令" << str.str().c_str());
  try {
    auto t = boost::process::system(str.str().c_str());
  } catch (const std::exception& err) {
    DOODLE_LOG_WARN(err.what() << '\n');
    return false;
  }

  //获得信息
  auto result = p_cacheFilePath.front().parent_path() / "doodle.json";
  if (boost::filesystem::exists(result)) {
    nlohmann::json root{};
    boost::filesystem::ifstream stream{result, std::ios_base::in};
    try {
      stream >> root;
    } catch (const nlohmann::json::parse_error& err) {
      DOODLE_LOG_ERROR(err.what());
      return false;
    }

    std::string result{};
    result.append("\n名称已经自动修复");
    for (const auto& item : root) {
      result.append("\n网格名称 ：")
          .append(item["name"].get<std::string>())
          .append("\n五边面： ")
          .append(item["PentagonalSurface"].get<bool>() ? "true" : "false")
          .append("\n多Uv： ");
      if (item["map"]["MultipleUvMap"].get<bool>()) {
        result.append("是");
      } else {
        result.append("否");
      }
      result.append("------\n");
    }
    p_info_ptr_->setInfoP(result);

    return true;
  }
  return false;
}

void mayaArchive::setUseCustomPath(const dpathPtr& custom_path) {
}

void mayaArchive::insertDB() {
  p_info_ptr_->setFileList(p_ServerPath);
  if (p_info_ptr_->getInfoP().empty()) {
    p_info_ptr_->setInfoP("maya动画文件");
  }
  if (p_info_ptr_->isInsert())
    p_info_ptr_->updateSQL();
  else
    p_info_ptr_->insert();
}
void mayaArchive::_generateFilePath() {
  if (!p_soureFile.empty()) {
    if (isServerzinsideDir(p_soureFile.front())) {
      auto str = coreSet::toIpPath(p_soureFile.front().generic_string());
      p_ServerPath.push_back(str);
      return;
    }
    p_ServerPath.push_back(
        p_info_ptr_->generatePath(
            "Scenefiles",
            boost::filesystem::extension(p_soureFile[0])));
  } else if (!p_info_ptr_->getFileList().empty()) {
    for (auto&& item : p_info_ptr_->getFileList()) {
      p_ServerPath.push_back(item);
    }
  }
}

DOODLE_NAMESPACE_E
