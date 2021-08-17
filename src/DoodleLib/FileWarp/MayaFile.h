#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

#include <boost/signals2.hpp>
namespace doodle {
class DOODLELIB_API MayaFile : public details::no_copy {
 private:
  FSys::path p_path;
  long_term_ptr p_term;

  [[nodiscard]] static FSys::path createTmpFile(const std::string& in_resource_path);
  [[nodiscard]] static FSys::path warit_tmp_file(const std::string& in_string);
  bool checkFile();
  bool run_comm(const std::wstring& in_com) const;

 public:
  explicit MayaFile(FSys::path mayaPath = {});
  [[nodiscard]] long_term_ptr get_term() const;
  /**
   * 导出maya文件中的fbx
   * @param file_path 输入的maya路径
   * @param export_path 导出的路径
   * @return 是否导出成功
   */
  bool exportFbxFile(const FSys::path& file_path, const FSys::path& export_path = {}) const;
  bool batchExportFbxFile(const std::vector<FSys::path>& file_path) const;
  bool qcloth_sim_file(const FSys::path& file_path) const;
  bool batch_qcloth_sim_file(const std::vector<FSys::path>& file_path) const;

  [[nodiscard]] static bool is_maya_file(const FSys::path& in_path);
};

}  // namespace doodle
