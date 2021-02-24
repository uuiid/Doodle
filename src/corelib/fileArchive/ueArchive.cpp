#include "ueArchive.h"

#include <corelib/assets/assType.h>
#include <corelib/assets/assfilesqlinfo.h>
#include <corelib/core/coreset.h>
#include <corelib/core/Project.h>
#include <corelib/fileDBInfo/filesqlinfo.h>
#include <corelib/filesystem/FileSystem.h>
#include <corelib/filesystem/fileSync.h>
DOODLE_NAMESPACE_S
ueArchive::ueArchive(fileSqlInfoPtr data)
    : p_info_(std::move(data)) {}

void ueArchive::insertDB() {
  dpathList list = p_ServerPath;
  list.push_back(p_ServerPath.front().parent_path() / DOODLE_CONTENT);
  p_info_->setFileList(list);
  auto info = std::dynamic_pointer_cast<assFileSqlInfo>(p_info_);

  info->setAssType(assType::findType(assType::e_type::UE4, true));
  if (p_info_->getInfoP().empty())
    p_info_->setInfoP("ue场景文件");
}

void ueArchive::imp_generateFilePath() {
  if (!p_soureFile.empty()) {
    //先检查一下是不是自定义的路径  是的话直接不拷贝
    if (isServerzinsideDir(p_soureFile.front())) {
      auto str = coreSet::toIpPath(p_soureFile.front().generic_string());
      p_ServerPath.push_back(str);
      return;
    }

    //如果不是的话就直接开始上传
    if (isServerzinsideDir(p_soureFile.front())) {
      p_ServerPath.push_back(p_soureFile.front());
    }
    p_ServerPath.push_back(p_info_->generatePath(
        "Scenefiles", boost::filesystem::extension(p_soureFile.front())));
  } else if (!p_info_->getFileList().empty())
    for (const auto &item : p_info_->getFileList()) p_ServerPath.push_back(item);
  //
  //  if (p_soureFile.size() == 1)
  //      p_soureFile.push_back(p_soureFile.front().parent_path() / "Content");
}
void ueArchive::imp_updata(const dpathList &pathList) {
  assert(p_ServerPath.size() == p_cacheFilePath.size());
  //这里先上传ue文件 比较一下是不是自定义路径如果是就直接算是上传完成
  if (p_soureFile.front() == coreSet::getSet().getProject()->Root() / p_ServerPath.front())
    return;
  fileArchive::imp_updata({pathList.front()});

  //这里开始同步ue文件
  auto option = std::make_shared<fileDowUpdateOptions>();
  option->setlocaPath(p_soureFile.front().parent_path() / DOODLE_CONTENT);
  option->setremotePath(p_ServerPath.front().parent_path() / DOODLE_CONTENT);
  auto &fsys = DfileSyntem::get();
  fsys.upload(option);
}
void ueArchive::imp_down(const fileSys::path &localPath) {
  auto option = std::make_shared<fileDowUpdateOptions>();
  option->setlocaPath(localPath.parent_path() / DOODLE_CONTENT);
  option->setremotePath(p_ServerPath.back());
  auto &fsys = DfileSyntem::get();
  fsys.down(option);

  p_ServerPath.pop_back();
  p_cacheFilePath.pop_back();
  fileArchive::imp_down(localPath);
}
DOODLE_NAMESPACE_E
