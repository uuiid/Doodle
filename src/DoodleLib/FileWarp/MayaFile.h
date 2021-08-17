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
  /**
   * @brief 
   * 
   * 
   * 我一直做测试， 终于找到原因了：
   * 1、引用的文件必须和动画文件帧率相同，比如一个引用是24帧每秒， 动画是25 ，就会会卡死
   * 2、两个重复的一样的引用，这个时候会卡死，
   * 3、导出abc时不可以从1001开始，必须从解算的起始帧开始，要不然会出现不解算直接停留在第一帧
   * 
   * @param file_path 需要解算的文件路径
   * @return true 
   * @return false 
   */
  bool qcloth_sim_file(const FSys::path& file_path) const;
  bool batch_qcloth_sim_file(const std::vector<FSys::path>& file_path) const;

  [[nodiscard]] static bool is_maya_file(const FSys::path& in_path);
};

}  // namespace doodle
