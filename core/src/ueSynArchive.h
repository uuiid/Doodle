//
// Created by teXiao on 2020/11/11.
//
#include <core_global.h>
#include <src/fileArchive.h>


CORE_NAMESPACE_S
class ueSynArchive : public fileArchive {
 public:
  ueSynArchive();
  dpath down() override;
  bool update() override;
 protected:
  void insertDB() override;
  void _generateFilePath() override;
 private:
  freeSynWrapPtr p_syn;
};

CORE_NAMESPACE_E