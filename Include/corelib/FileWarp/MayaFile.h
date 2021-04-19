#include <corelib/core_global.h>

#include <corelib/threadPool/LongTerm.h>
#include <boost/signals2.hpp>
DOODLE_NAMESPACE_S
class CORE_API MayaFile : public LongTerm {
 private:
  FSys::path p_path;

  const FSys::path createTmpFile() const;
  bool checkFile();

 public:
  MayaFile(FSys::path mayaPath = {});
  bool exportFbxFile(const FSys::path& file_path, const FSys::path& export_path = {}) const;
  bool batchExportFbxFile(const std::vector<FSys::path>& file_path) const;

  DOODLE_DISABLE_COPY(MayaFile)

};

DOODLE_NAMESPACE_E