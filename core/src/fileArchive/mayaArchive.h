#pragma once

#include "core_global.h"

#include "src/fileArchive/fileArchive.h"

CORE_NAMESPACE_S

class CORE_API mayaArchive : public fileArchive {
 public:
  explicit mayaArchive(fileSqlInfoPtr shot_data);

  virtual bool useUpdataCheck() const override;
  virtual bool updataCheck() const override;

  virtual bool useDowndataCheck() const override;
  virtual bool downdataCheck() const override;

  void setUseCustomPath(const dpathPtr& custom_path);

 protected:
  void insertDB() override;
  void _generateFilePath() override;

 private:
  fileSqlInfoPtr p_info_ptr_;
};

CORE_NAMESPACE_E