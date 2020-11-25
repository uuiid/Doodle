//
// Created by teXiao on 2020/11/11.
//
#include <core_global.h>
#include <src/fileArchive.h>


CORE_NAMESPACE_S
class CORE_API ueSynArchive : public fileArchive {
 public:
  ueSynArchive();
  dpath syn(const shotPtr& shot_);
  bool update() override;
  bool makeDir(const episodesPtr &episodes_ptr);

 protected:
  void insertDB() override;
  void _generateFilePath() override;
 private:
  freeSynWrapPtr p_syn;
  synPathListPtr synpart;


};

CORE_NAMESPACE_E