#include <core_global.h>

#include <boost/signals2.hpp>
DOODLE_NAMESPACE_S
class CORE_API MayaFile {
 private:
  FSys::path p_path;

  void findMaya();
  const FSys::path createTmpFile() const;
  bool checkFile();

 public:
  MayaFile(FSys::path mayaPath = {});
  bool exportFbxFile(const FSys::path& file_path, const FSys::path& export_path = {}) const;
  bool batchExportFbxFile(const std::vector<FSys::path>& file_path) const;

  DOODLE_DISABLE_COPY(MayaFile)

  boost::signals2::signal<void(int)> progress;
  boost::signals2::signal<void(const std::string& message)> messagResult;
  boost::signals2::signal<void()> finished;
};

DOODLE_NAMESPACE_E