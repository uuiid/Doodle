#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

#include <DoodleLib/threadPool/LongTerm.h>
#include <boost/signals2.hpp>
namespace doodle {
class DOODLELIB_API MayaFile : public LongTerm {
 private:
  FSys::path p_path;

  FSys::path createTmpFile() const;
  bool checkFile();

 public:
  MayaFile(FSys::path mayaPath = {});
  bool exportFbxFile(const FSys::path& file_path, const FSys::path& export_path = {}) const;
  bool batchExportFbxFile(const std::vector<FSys::path>& file_path) const;

  static [[nodiscard]] bool is_maya_file(const FSys::path& in_path);
  DOODLE_DISABLE_COPY(MayaFile)

};

}
