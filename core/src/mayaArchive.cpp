#include "mayaArchive.h"
#include "shotfilesqlinfo.h"

#include "Logger.h"
CORE_NAMESPACE_S
mayaArchive::mayaArchive(shotInfoPtr &shot_data)
    : p_info_ptr_(shot_data) {}

void mayaArchive::insertDB() {
  p_info_ptr_->setFileList({p_Path});
  p_info_ptr_->insert();
}
void mayaArchive::_generateFilePath() {

  if (!p_soureFile.isEmpty())
    p_Path = p_info_ptr_->generatePath("Scenefiles", p_soureFile[0].suffix());
  else if (!p_info_ptr_->getFileList().isEmpty()) {
    p_Path = p_info_ptr_->generatePath("Scenefiles", p_info_ptr_->getFileList()[0].suffix());
    p_soureFile = p_info_ptr_->getFileList();
  }
}
bool mayaArchive::exportFbx() {
  auto info = down()[0];

  const auto mayapath = QString(R"("C:\\Program Files\\Autodesk\\Maya2018\\bin\\mayapy.exe")");
  auto filePath = info.filePath();
  DOODLE_LOG_INFO << "导出文件" << filePath;

  auto comm = QString("%1 %2 --path %3 --name %4 --version %5 --suffix %6 --exportpath %7")
      .arg(mayapath)//maya py 解释器位置 -->1
      .arg(filePath)//导出脚本位置           -->2
      .arg(info.path())//导出到文件的位置中--3
      .arg(info.baseName())//导出的名称  --4
      .arg(p_info_ptr_->getVersionP())//版本 --5
      .arg("." + info.suffix())//文件后缀 -- 6
      .arg(info.path());
  DOODLE_LOG_INFO << "导出命令" << comm;
  return true;
}
CORE_NAMESPACE_E
