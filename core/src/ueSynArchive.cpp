//
// Created by teXiao on 2020/11/11.
//

#include "ueSynArchive.h"

#include <src/coreset.h>
#include <src/freeSynWrap.h>
#include <boost/format.hpp>
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

  for (auto &item : synpart) {
    item.local = set.getSynPathLocale() / set.projectName().second / item.local / DOODLE_CONTENT / "shot";
    item.server = set.getShotRoot() / set.getDepartment() / item.server / DOODLE_CONTENT / "shot";
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
      item.local = set.getSynPathLocale() / set.projectName().second / item.local / DOODLE_CONTENT / "shot";
      item.server = set.getShotRoot() / "VFX" / item.server / DOODLE_CONTENT / "shot";
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
CORE_NAMESPACE_E