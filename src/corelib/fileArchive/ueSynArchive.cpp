//
// Created by teXiao on 2020/11/11.
//

#include "ueSynArchive.h"

#include <loggerlib/Logger.h>
#include <corelib/shots/episodes.h>

#include <corelib/filesystem/FileSystem.h>

#include <corelib/assets/assfilesqlinfo.h>
#include <corelib/core/coreDataManager.h>
#include <corelib/core/coreset.h>
#include <corelib/shots/episodes.h>
#include <corelib/exeWrap/freeSynWrap.h>
#include <corelib/shots/shot.h>
#include <corelib/sysData/synData.h>

#include <boost/format.hpp>
#include <boost/numeric/conversion/cast.hpp>

DOODLE_NAMESPACE_S
ueSynArchive::ueSynArchive()
    : fileArchive(), p_syn(std::make_shared<freeSynWrap>()), synpart() {}
void ueSynArchive::insertDB() {}
void ueSynArchive::imp_generateFilePath() {}

dpath ueSynArchive::syn(const episodesPtr &episodes_ptr, const shotPtr &shot_ptr) {
  auto &set = coreSet::getSet();
  boost::format str("/03_Workflow/Assets/%s/backup");
  str % set.getDepartment();

  auto k_synData = synData::getAll(episodes_ptr);

  synpart = k_synData->getSynDir(true);
  if (synpart.empty()) return {};

  dstring k_shotVFXstr   = "*\\VFX\\*";
  dstring k_shotLightstr = "*";
  if (shot_ptr) {
    boost::format shotFlliter(R"(*c%04i\Checkpoint\VFX\*)");
    boost::format k_shotLight(R"(*c%04i)");

    k_shotVFXstr   = (shotFlliter % shot_ptr->getShot()).str();
    k_shotLightstr = (k_shotLight % shot_ptr->getShot()).str();
  }

  p_syn->addSynFile(synpart);
  p_syn->setVersioningFolder(freeSynWrap::syn_set::twoWay, str.str());
  if (set.getDepartment() == "VFX") {
    //设置同步方式
    p_syn->addInclude({k_shotVFXstr});

  } else if (set.getDepartment() == "Light") {
    //同步light镜头
    for (int i = 0; i < synpart.size(); ++i) {
      p_syn->addSubSynchronize(i, freeSynWrap::syn_set::upload, str.str());
      p_syn->addSubIncludeExclude(i, {k_shotLightstr}, {k_shotVFXstr});
    }

    //下载vfx镜头
    auto syn_part_vfx = k_synData->getSynDir(false);
    for (auto &item : syn_part_vfx) {
      item.local  = set.getSynPathLocale() / set.projectName().second / item.local;
      item.server = set.getAssRoot() / "VFX" / item.server;
    }
    p_syn->addSynFile(syn_part_vfx);

    for (size_t i = syn_part_vfx.size();
         i < syn_part_vfx.size() + synpart.size(); ++i) {
      p_syn->addSubSynchronize(boost::numeric_cast<int>(i), freeSynWrap::syn_set::down, str.str());
      p_syn->addSubIncludeExclude(boost::numeric_cast<int>(i), {k_shotVFXstr}, {});
    }
  }
  p_syn->run();

  return {};
}

bool ueSynArchive::update() {
  return false;
}
bool ueSynArchive::makeDir(const episodesPtr &episodes_ptr) {
  auto synClass = synData::getAll(episodes_ptr);
  auto assClass = coreDataManager::get().getAssClassPtr();
  //复制文件的来源
  auto soure_ptr = coreDataManager::get().getAssInfoPtr();

  //验证各种指针
  if (!episodes_ptr) return false;
  if (synClass->isNULL()) {
    synClass->setEpisodes(episodes_ptr);
    synClass->insert();
  }
  if (!soure_ptr) return false;

  auto create_path = synClass->push_back(assClass);

  synClass->updateSQL();

  auto &set = coreSet::getSet();

  dstringList list;
  auto &session = DfileSyntem::get();
  //复制灯光需要的文件夹
  auto server = set.getPrjectRoot() / set.getAssRoot() / set.getDepartment() /
                *create_path;
  auto copy_path_list = soure_ptr->getFileList();
  if (copy_path_list.size() == 1) {
    auto path = copy_path_list.front();
    session.copy(set.getPrjectRoot() / path, server / path.filename());
    session.copy(set.getPrjectRoot() / path.parent_path() / DOODLE_CONTENT, server / DOODLE_CONTENT);
  } else if (copy_path_list.size() == 2) {
    for (auto &copy_path : copy_path_list) {
      session.copy(set.getPrjectRoot() / copy_path, server / copy_path.filename());
    }
  }

  for (auto &&path : soure_ptr->getFileList()) {
  }
  //创建灯光需要的文件夹
  boost::format shot(DOODLE_SHFORMAT);
  dstringList listcreates{"Checkpoint", "Light", "Ren"};
  dstringList listDep{"Light", "VFX"};
  server = set.getAssRoot() / set.getDepartment() / *create_path /
           DOODLE_CONTENT / "shot" / episodes_ptr->getEpisdes_str();
  session.createDir(server.generic_string());

  for (int kI = 0; kI < 120; ++kI) {
    auto ks1 = server / (shot % kI).str();
    session.createDir({ks1.generic_string()});

    for (const auto &listcreate : listcreates) {
      auto ks3 = ks1 / listcreate;
      session.createDir({ks3.generic_string()});

      if (listcreate == *listcreates.begin()) {
        for (const auto &list_dep : listDep) {
          auto ks2 = ks3 / list_dep;
          session.createDir({ks2.generic_string()});
        }
      }
    }
  }
  return true;
}
DOODLE_NAMESPACE_E