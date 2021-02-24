//
// Created by teXiao on 2020/11/11.
//

#include "ueSynArchive.h"

#include <loggerlib/Logger.h>
#include <corelib/shots/episodes.h>

#include <corelib/filesystem/FileSystem.h>
#include <corelib/filesystem/fileSync.h>

#include <corelib/assets/assfilesqlinfo.h>
#include <corelib/core/coreDataManager.h>
#include <corelib/core/coreset.h>
#include <corelib/shots/episodes.h>
#include <corelib/shots/shot.h>
#include <corelib/sysData/synData.h>

#include <boost/format.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <corelib/threadPool/ThreadPool.h>

DOODLE_NAMESPACE_S
ueSynArchive::ueSynArchive()
    : fileArchive(), synpart() {}
void ueSynArchive::insertDB() {}
void ueSynArchive::imp_generateFilePath() {}

fileSys::path ueSynArchive::syn(const episodesPtr &episodes_ptr, const shotPtr &shot_ptr) {
  // auto &set = coreSet::getSet();
  // boost::format str("/03_Workflow/Assets/%s/backup");
  // str % set.getDepartment();

  // auto k_synData = synData::getAll(episodes_ptr);

  // //获得同步文件夹对
  // synpart = k_synData->getSynDir(true);
  // if (synpart.empty()) return {};

  // // 添加同步文件夹过滤器
  // dstring k_shotVFXstr   = "*/VFX/*";
  // dstring k_shotLightstr = ".";
  // if (shot_ptr) {
  //   boost::format shotFlliter(R"(*c%04i/Checkpoint/VFX/*)");
  //   boost::format k_shotLight(R"(*c%04i)");

  //   k_shotVFXstr   = (shotFlliter % shot_ptr->getShot()).str();
  //   k_shotLightstr = (k_shotLight % shot_ptr->getShot()).str();
  // }
  // // 开始同步文件夹
  // auto lists = std::vector<std::shared_ptr<fileDowUpdateOptions>>{};
  // for (auto &&iter : synpart) {
  //   auto option = std::make_shared<fileDowUpdateOptions>();
  //   option->setbackupPath(str.str());
  //   if (set.getDepartment() == "VFX") {
  //     option->setlocaPath(iter.local);
  //     option->setremotePath(iter.server);
  //     option->setInclude({std::make_shared<std::regex>(k_shotVFXstr)});
  //   } else if (set.getDepartment() == "Light") {
  //     option->setlocaPath(iter.local);
  //     option->setremotePath(iter.server);

  //     option->setInclude({std::make_shared<std::regex>(k_shotLightstr)});
  //     option->setExclude({std::make_shared<std::regex>(k_shotVFXstr)});
  //   }
  //   option->setbackupPath(str.str());
  //   lists.push_back(option);
  // }
  // if (set.getDepartment() == "Light") {
  //   auto syn_part_vfx = k_synData->getSynDir(false);
  //   for (auto &&item : syn_part_vfx) {
  //     //下载vfx镜头
  //     auto option_light = std::make_shared<fileDowUpdateOptions>();
  //     //设置背负路径
  //     option_light->setbackupPath(str.str());
  //     option_light->setlocaPath(set.getSynPathLocale() / set.projectName().second / item.local);
  //     option_light->setremotePath(item.server = set.getAssRoot() / "VFX" / item.server);

  //     option_light->setInclude({std::make_shared<std::regex>(k_shotVFXstr)});
  //     option_light->setbackupPath(str.str());
  //     lists.push_back(option_light);
  //   }
  // }
  // for (auto &&item : lists) {
  //   DfileSyntem::get().down(item);
  // }
  return {};
}

bool ueSynArchive::update(const episodesPtr &episodes_ptr, const shotPtr &shot_ptr) {
  // auto &set = coreSet::getSet();
  // boost::format str("/03_Workflow/Assets/%s/backup");
  // str % set.getDepartment();

  // auto k_synData = synData::getAll(episodes_ptr);

  // //获得同步文件夹对
  // synpart = k_synData->getSynDir(true);
  // if (synpart.empty()) return {};

  // // 添加同步文件夹过滤器
  // dstring k_shotVFXstr   = "/VFX/";
  // dstring k_shotLightstr = ".";
  // if (shot_ptr) {
  //   boost::format shotFlliter(R"(c%04i/Checkpoint/VFX/)");
  //   boost::format k_shotLight(R"(c%04i)");

  //   k_shotVFXstr   = (shotFlliter % shot_ptr->getShot()).str();
  //   k_shotLightstr = (k_shotLight % shot_ptr->getShot()).str();
  // }
  // // 开始同步文件夹
  // auto lists = std::vector<std::shared_ptr<fileDowUpdateOptions>>{};
  // for (auto &&iter : synpart) {
  //   auto option = std::make_shared<fileDowUpdateOptions>();
  //   option->setbackupPath(str.str());
  //   if (set.getDepartment() == "VFX") {
  //     option->setlocaPath(iter.local);
  //     option->setremotePath(iter.server);
  //     option->setInclude({std::make_shared<std::regex>(k_shotVFXstr)});
  //   }
  //   //这一部分是上传灯光中的特效文件到灯光文件夹  需要
  //   else if (set.getDepartment() == "Light") {
  //     option->setlocaPath(iter.local);
  //     option->setremotePath(iter.server);

  //     option->setInclude({std::make_shared<std::regex>(k_shotLightstr)});
  //     option->setExclude({std::make_shared<std::regex>(k_shotVFXstr)});
  //   }
  //   option->setbackupPath(str.str());
  //   lists.push_back(option);
  // }
  // if (set.getDepartment() == "Light") {
  //   auto syn_part_vfx = k_synData->getSynDir(false);
  //   for (auto &&item : syn_part_vfx) {
  //     //下载vfx镜头
  //     auto option_light = std::make_shared<fileDowUpdateOptions>();
  //     //设置背负路径
  //     option_light->setbackupPath(str.str());
  //     option_light->setlocaPath(set.getSynPathLocale() / set.projectName().second / item.local);
  //     option_light->setremotePath(item.server = set.getAssRoot() / "VFX" / item.server);

  //     option_light->setInclude({std::make_shared<std::regex>(k_shotVFXstr)});
  //     option_light->setbackupPath(str.str());
  //     lists.push_back(option_light);
  //   }
  // }
  // for (auto &&item : lists) {
  //   DfileSyntem::get().upload(item);
  // }
  return true;
}

fileSys::path ueSynArchive::down(const episodesPtr &episodes_ptr, const shotPtr &shot_ptr) {
  // auto &set = coreSet::getSet();
  // boost::format str("/03_Workflow/Assets/%s/backup");
  // str % set.getDepartment();

  // auto k_synData = synData::getAll(episodes_ptr);

  // //获得同步文件夹对
  // synpart = k_synData->getSynDir(true);
  // if (synpart.empty()) return {};

  // // 添加同步文件夹过滤器
  // dstring k_shotVFXstr   = "/VFX/";
  // dstring k_shotLightstr = ".";
  // if (shot_ptr) {
  //   boost::format shotFlliter(R"(c%04i/Checkpoint/VFX/)");
  //   boost::format k_shotLight(R"(c%04i)");

  //   k_shotVFXstr   = (shotFlliter % shot_ptr->getShot()).str();
  //   k_shotLightstr = (k_shotLight % shot_ptr->getShot()).str();
  // }
  // // 开始同步文件夹
  // auto lists = std::vector<std::shared_ptr<fileDowUpdateOptions>>{};
  // for (auto &&iter : synpart) {
  //   auto option = std::make_shared<fileDowUpdateOptions>();
  //   option->setbackupPath(str.str());
  //   if (set.getDepartment() == "VFX") {
  //     option->setlocaPath(iter.local);
  //     option->setremotePath(iter.server);
  //     option->setInclude({std::make_shared<std::regex>(k_shotVFXstr)});

  //     option->setbackupPath(str.str());
  //     lists.push_back(option);
  //   }
  //   //这一部分是下载灯光文件,默认情况是不下载
  //   //  else if (set.getDepartment() == "Light") {
  //   //   option->setlocaPath(iter.local);
  //   //   option->setremotePath(iter.server);

  //   //   option->setInclude({std::make_shared<std::regex>(k_shotLightstr)});
  //   //   option->setExclude({std::make_shared<std::regex>(k_shotVFXstr)});
  //   // }
  // }
  // if (set.getDepartment() == "Light") {
  //   auto syn_part_vfx = k_synData->getSynDir(false);
  //   for (auto &&item : syn_part_vfx) {
  //     //下载vfx镜头
  //     auto option_light = std::make_shared<fileDowUpdateOptions>();
  //     //设置备份路径
  //     option_light->setlocaPath(set.getSynPathLocale() / set.projectName().second / item.local);
  //     option_light->setremotePath(item.server = set.getAssRoot() / "VFX" / item.server);

  //     option_light->setInclude({std::make_shared<std::regex>(k_shotVFXstr)});
  //     lists.push_back(option_light);
  //   }
  // }
  // for (auto &&item : lists) {
  //   DfileSyntem::get().down(item);
  // }
  return {};
}
bool ueSynArchive::makeDir(const episodesPtr &episodes_ptr) {
  // auto synClass = synData::getAll(episodes_ptr);
  // auto assClass = coreDataManager::get().getAssClassPtr();
  // //复制文件的来源
  // auto soure_ptr = coreDataManager::get().getAssInfoPtr();

  // //验证各种指针
  // if (!episodes_ptr) return false;
  // if (synClass->isNULL()) {
  //   synClass->setEpisodes(episodes_ptr);
  //   synClass->insert();
  // }
  // if (!soure_ptr) return false;

  // auto create_path = synClass->push_back(assClass);

  // synClass->updateSQL();

  // auto &set = coreSet::getSet();

  // dstringList list;
  // auto &session = DfileSyntem::get();
  // //复制灯光需要的文件夹
  // auto server = set.getAssRoot() / set.getDepartment() /
  //               *create_path;
  // auto copy_path_list = soure_ptr->getFileList();
  // if (copy_path_list.size() == 1) {
  //   auto path = copy_path_list.front();
  //   session.copy(path, server / path.filename());
  //   session.copy(path.parent_path() / DOODLE_CONTENT, server / DOODLE_CONTENT);
  // } else if (copy_path_list.size() == 2) {
  //   for (auto &copy_path : copy_path_list) {
  //     session.copy(copy_path, server / copy_path.filename());
  //   }
  // }

  // for (auto &&path : soure_ptr->getFileList()) {
  // }
  // //创建灯光需要的文件夹
  // boost::format shot(DOODLE_SHFORMAT);
  // dstringList listcreates{"Checkpoint", "Light", "Ren"};
  // dstringList listDep{"Light", "VFX"};
  // server = set.getAssRoot() / set.getDepartment() / *create_path /
  //          DOODLE_CONTENT / "shot" / episodes_ptr->getEpisdes_str();

  // for (int kI = 0; kI < 120; ++kI) {
  //   auto ks1 = server / (shot % kI).str();
  //   for (const auto &listcreate : listcreates) {
  //     auto ks3 = ks1 / listcreate;
  //     session.createDir(ks3);
  //     if (listcreate == *listcreates.begin()) {
  //       for (const auto &list_dep : listDep) {
  //         auto ks2 = ks3 / list_dep;
  //         session.createDir(ks2);
  //       }
  //     }
  //   }
  // }
  return true;
}
DOODLE_NAMESPACE_E