#include <core_global.h>

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
};

DOODLE_NAMESPACE_E