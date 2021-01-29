//
// Created by teXiao on 2020/11/23.
//

#include <corelib/core_global.h>
#include <corelib/fileArchive/fileArchive.h>

DOODLE_NAMESPACE_S

class CORE_API movieEpsArchive : public fileArchive {
 public:
  explicit movieEpsArchive(shotInfoPtr eps);
  bool update() override;

 protected:
  void insertDB() override;
  void imp_generateFilePath() override;
 private:
  shotInfoPtr p_info_ptr_;
 private:
  bool epsMove();
};


DOODLE_NAMESPACE_E
