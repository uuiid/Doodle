#include "ueArchive.h"

#include <src/assets/assType.h>
#include <src/assets/assfilesqlinfo.h>

#include "src/core/coreset.h"
#include "src/fileDBInfo/filesqlinfo.h"
#include "src/exeWrap/freeSynWrap.h"
CORE_NAMESPACE_S
ueArchive::ueArchive(doCore::fileSqlInfoPtr data)
    : p_info_(std::move(data)), p_syn(std::make_shared<freeSynWrap>()) {}
void ueArchive::insertDB() {
  dpathList list = p_ServerPath;
  list.push_back(p_ServerPath.front().parent_path() / DOODLE_CONTENT);
  p_info_->setFileList(list);
  auto info = std::dynamic_pointer_cast<assFileSqlInfo>(p_info_);
  info->setAssType(assType::findType(assType::e_type::UE4, true));
  if (p_info_->getInfoP().empty())
    p_info_->setInfoP("ue场景文件");

  if (p_info_->isInsert())
    p_info_->updateSQL();
  else
    p_info_->insert();
}
void ueArchive::_generateFilePath() {
  if (!p_soureFile.empty()) {
    p_ServerPath.push_back(p_info_->generatePath(
        "Scenefiles", boost::filesystem::extension(p_soureFile.front())));
  } else if (!p_info_->getFileList().empty())
    for (const auto &item : p_info_->getFileList()) p_ServerPath.push_back(item);
  //
  //  if (p_soureFile.size() == 1)
  //      p_soureFile.push_back(p_soureFile.front().parent_path() / "Content");
}
void ueArchive::_updata(const dpathList &pathList) {
  assert(p_ServerPath.size() == p_cacheFilePath.size());
  fileArchive::_updata({pathList.front()});
  synPath_struct syn_path_struct{};
  syn_path_struct.local  = p_soureFile.front().parent_path() / DOODLE_CONTENT;
  syn_path_struct.server = p_ServerPath.front().parent_path() / DOODLE_CONTENT;
  p_syn->addSynFile({syn_path_struct});
  p_syn->setVersioningFolder(freeSynWrap::syn_set::upload,
                             p_ServerPath.front().parent_path() / DOODLE_BACKUP);
  p_syn->run();
}
void ueArchive::_down(const dpath &localPath) {
  synPath_struct syn_path_struct{};
  syn_path_struct.server = p_ServerPath.back();
  syn_path_struct.local  = localPath.parent_path() / DOODLE_CONTENT;
  p_syn->addSynFile({syn_path_struct});
  p_syn->setVersioningFolder(freeSynWrap::syn_set::down,
                             p_ServerPath.front().parent_path() / DOODLE_BACKUP);
  p_syn->run();
  p_ServerPath.pop_back();
  p_cacheFilePath.pop_back();
  fileArchive::_down(localPath);
}
CORE_NAMESPACE_E
