//
// Created by teXiao on 2020/11/11.
//

#include "ueSynArchive.h"

#include <src/coreset.h>
#include <src/freeSynWrap.h>
#include <boost/format.hpp>
#include <src/ftphandle.h>
#include <src/ftpsession.h>
CORE_NAMESPACE_S
ueSynArchive::ueSynArchive()
    : fileArchive(),
      p_syn(std::make_shared<freeSynWrap>()) {

}
void ueSynArchive::insertDB() {

}
void ueSynArchive::_generateFilePath() {

}
dpath ueSynArchive::down() {
  auto &set = coreSet::getSet();
  boost::format str("/03_Workflow/Assets/%s/backup");
  str % set.getDepartment();
  auto synpart = set.getSynDir();

  boost::format eps("EP_%20i");
  eps % set.getSyneps();
  for (auto &item : synpart) {
    item.local = set.getSynPathLocale() / set.projectName().second / eps.str() / item.local / DOODLE_CONTENT / "shot";
    item.server = set.getShotRoot() / set.getDepartment() / eps.str() / item.server / DOODLE_CONTENT / "shot";
  }

  p_syn->addSynFile(synpart);
  if (set.getDepartment() == "VFX") {
    //设置同步方式
    p_syn->setVersioningFolder(freeSynWrap::syn_set::twoWay, str.str());
    p_syn->addInclude({"*\\VFX\\*"});
  } else if (set.getDepartment() == "Light") {
    //同步light镜头
    for (int i = 0; i < synpart.size(); ++i) {
      p_syn->addSubSynchronize(i, freeSynWrap::syn_set::twoWay, str.str());
      p_syn->addSubIncludeExclude(i, {"*"}, {"*\\VFX\\*"});
    }

    //下载vfx镜头
    auto syn_part_vfx = set.getSynDir();
    for (auto &item : syn_part_vfx) {
      item.local = set.getSynPathLocale() / set.projectName().second / eps.str() / item.local / DOODLE_CONTENT / "shot";
      item.server = set.getShotRoot() / "VFX" / item.server / eps.str() / DOODLE_CONTENT / "shot";
    }
    p_syn->addSynFile(syn_part_vfx);
    for (int i = syn_part_vfx.size(); i < syn_part_vfx.size() + synpart.size(); ++i) {
      p_syn->addSubSynchronize(i, freeSynWrap::syn_set::down, str.str());
      p_syn->addSubIncludeExclude(i, {"*\\VFX\\*"}, {});
    }
  } else {
    return fileArchive::down();
  }
  p_syn->run();

  return {};
}
bool ueSynArchive::update() {
  down();
  return true;
}
bool ueSynArchive::makeDir() {
  auto &set = coreSet::getSet();
  auto synpart = set.getSynDir();
  boost::format eps("EP_%20i");
  eps % set.getSyneps();
  dstringList list;

  boost::format eps2("Ep%30i");
  eps2 % set.getSyneps();

  boost::format shot("Sc%40i");

  dstringList listcreates{"Checkpoint",
                          "Light",
                          "Ren"};
  dstringList listDep{"Light",
                      "VFX"};
  for (auto &item : synpart) {
    auto server = set.getShotRoot()
        / set.getDepartment()
        / eps.str()
        / item.server
        / DOODLE_CONTENT
        / "shot"
        / eps2.str();
    for (int kI = 0; kI < 120; ++kI) {
      auto ks1 = server / (shot % kI).str();
      for (const auto &listcreate : listcreates) {
        if (listcreate == *listcreates.begin()) {
          for (const auto &list_dep : listDep) {
            auto ks2 = ks1 / listcreate / list_dep;
            list.push_back(ks2.generic_string());
          }
          auto ks3 = ks1 / listcreate;
          list.push_back(ks3.generic_string());
        }
      }
    }
  }

  return doFtp::ftphandle::getFTP().session(
      set.getIpFtp(),
      21,
      set.getProjectname() + set.getUser_en(),
      set.getUser_en()
  )->createDir(list);
}
CORE_NAMESPACE_E