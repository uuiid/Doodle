#include "mayaArchive.h"
#include "shotfilesqlinfo.h"

CORE_NAMESPACE_S
mayaArchive::mayaArchive(shotInfoPtr & shot_data)
:p_info_ptr_(shot_data)
{}

void mayaArchive::insertDB() {
p_info_ptr_->setFileList({p_Path});
p_info_ptr_->insert();
}
void mayaArchive::_generateFilePath() {
p_Path = p_info_ptr_->generatePath("Scenefiles",p_soureFile[0].suffix());
}
CORE_NAMESPACE_E
