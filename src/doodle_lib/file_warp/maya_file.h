#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/lib_warp/json_warp.h>

#include <boost/signals2.hpp>
namespace doodle {
class DOODLELIB_API maya_file
    : public details::no_copy,
      public std::enable_shared_from_this<maya_file> {
 private:
  FSys::path p_path;

  static void write_maya_tool_file();
  [[nodiscard]] static FSys::path warit_tmp_file(const std::string& in_string);
  bool checkFile();
  static bool run_comm(const std::wstring& in_com, const long_term_ptr& in_term);

 public:
  class DOODLELIB_API qcloth_arg {
   public:
    qcloth_arg()  = default;
    ~qcloth_arg() = default;
    FSys::path sim_path;
    FSys::path qcloth_assets_path;
    FSys::path export_path;
    uuid uuid_p;
    bool only_sim;

    friend void to_json(nlohmann::json& nlohmann_json_j, const qcloth_arg& nlohmann_json_t) {
      nlohmann_json_j["path"]               = nlohmann_json_t.sim_path;
      nlohmann_json_j["export_path"]        = nlohmann_json_t.export_path;
      nlohmann_json_j["qcloth_assets_path"] = nlohmann_json_t.qcloth_assets_path;
      nlohmann_json_j["only_sim"]           = nlohmann_json_t.only_sim;
      nlohmann_json_j["uuid"]               = boost::lexical_cast<string>(nlohmann_json_t.uuid_p);
    };
  };
  using qcloth_arg_ptr = std::shared_ptr<qcloth_arg>;

  class DOODLELIB_API export_fbx_arg {
   public:
    export_fbx_arg()  = default;
    ~export_fbx_arg() = default;
    /**
     * @brief maya文件源路径(文件路径)
     *
     */
    FSys::path file_path;
    /**
     * @brief 导出文件的路径(目录)
     *
     */
    FSys::path export_path;
    /**
     * @brief 是否导出所有引用
     *
     */
    bool use_all_ref;
    friend void to_json(nlohmann::json& nlohmann_json_j, const export_fbx_arg& nlohmann_json_t) {
      nlohmann_json_j["path"]        = nlohmann_json_t.file_path;
      nlohmann_json_j["export_path"] = nlohmann_json_t.export_path;
      nlohmann_json_j["use_all_ref"] = nlohmann_json_t.use_all_ref;
    };
  };
  using export_fbx_arg_ptr = std::shared_ptr<export_fbx_arg>;

  explicit maya_file(FSys::path mayaPath = {});

  /**
   * 导出maya文件中的fbx
   * @param file_path 输入的maya路径
   * @param export_path 导出的路径
   * @return 是否导出成功
   */
  void export_fbx_file(const FSys::path& file_path, const FSys::path& export_path, const long_term_ptr& in_ptr);
  /**
   * 导出maya文件中的fbx
   * @param file_path 输入的maya路径
   * @param export_path 导出的路径
   * @return 是否导出成功
   */
  void export_fbx_file(const export_fbx_arg_ptr& in_arg, const long_term_ptr& in_ptr);

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
  void qcloth_sim_file(const qcloth_arg_ptr& in_arg, const long_term_ptr& in_ptr);

  [[nodiscard]] static bool is_maya_file(const FSys::path& in_path);
};

class DOODLELIB_API maya_file_async : public details::no_copy {
  std::shared_ptr<maya_file> p_maya_file;

 public:
  maya_file_async();

  long_term_ptr export_fbx_file(const FSys::path& file_path, const FSys::path& export_path = {});
  long_term_ptr export_fbx_file(maya_file::export_fbx_arg_ptr& in_arg);
  long_term_ptr qcloth_sim_file(maya_file::qcloth_arg_ptr& in_arg);
};

}  // namespace doodle
