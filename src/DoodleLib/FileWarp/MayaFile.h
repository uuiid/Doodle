#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/threadPool/long_term.h>

#include <boost/signals2.hpp>
namespace doodle {
class DOODLELIB_API MayaFile : public long_term, public details::no_copy {
 private:
  FSys::path p_path;

  [[nodiscard]] FSys::path createTmpFile() const;
  bool checkFile();

 public:
  explicit MayaFile(FSys::path mayaPath = {});
  bool exportFbxFile(const FSys::path& file_path, const FSys::path& export_path = {}) const;
  bool batchExportFbxFile(const std::vector<FSys::path>& file_path) const;

  [[nodiscard]] static bool is_maya_file(const FSys::path& in_path);
};

}  // namespace doodle
