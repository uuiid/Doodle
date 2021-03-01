#include <core_global.h>

DOODLE_NAMESPACE_S
class MayaFile {
 private:
  fileSys::path p_path;

  std::string findMaya();

 public:
  MayaFile(fileSys::path path);

  bool exportFbxFile(fileSys::path path);
  bool checkFile();
};

DOODLE_NAMESPACE_E