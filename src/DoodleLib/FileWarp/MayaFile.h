#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

#include <boost/signals2.hpp>
namespace doodle {
class DOODLELIB_API MayaFile
    : public details::no_copy {
 private:
  FSys::path p_path;
  long_term_ptr p_term;

  std::vector<long_term_ptr> p_term_list;
  std::vector<std::future<void>> p_futurn_list;

  static void write_maya_tool_file();
  [[nodiscard]] static FSys::path warit_tmp_file(const std::string& in_string);
  bool checkFile();
  bool run_comm(const std::wstring& in_com) const;

 public:
  class DOODLELIB_API qcloth_arg {
   public:
    qcloth_arg() = default;
    FSys::path sim_path;
    FSys::path qcloth_assets_path;
  };
  using qcloth_arg_ptr = std::unique_ptr<qcloth_arg>;

  explicit MayaFile(FSys::path mayaPath = {});
  virtual ~MayaFile();
  [[nodiscard]] long_term_ptr get_term() const;
  /**
   * 导出maya文件中的fbx
   * @param file_path 输入的maya路径
   * @param export_path 导出的路径
   * @return 是否导出成功
   */
  [[nodiscard]] long_term_ptr exportFbxFile(const FSys::path& file_path, const FSys::path& export_path = {});

  /**
   * @brief 批量解算qcloth 文件
   * 在使用时， 可以重复的调用， 由于是异步的，所以请接受返回信号，信号的所有权是自己本身
   *
   *
   * @note 导出abc时会重复导出一次，第一次用来创建缓存，第二次用来更改导出帧为1001，
   * 
   * @todo maya 导出时需要拍屏
   * @todo 公开寻找引用的路径ui
   * 
   * 我一直做测试， 终于找到原因了：
   * 1、引用的文件必须和动画文件帧率相同，比如一个引用是24帧每秒， 动画是25 ，就会卡死
   * 2、导出abc时不可以从1001开始，必须从解算的起始帧开始，要不然会出现不解算直接停留在第一帧
   * 
   * @param qcloth_arg_ptr 配置参数
   * @return true 
   * @return false 
   */
  [[nodiscard]] long_term_ptr qcloth_sim_file(qcloth_arg_ptr& in_arg);

  [[nodiscard]] static bool is_maya_file(const FSys::path& in_path);
};

}  // namespace doodle
