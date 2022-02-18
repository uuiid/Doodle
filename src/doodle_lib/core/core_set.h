#pragma once


#include <doodle_lib/core/ue4_setting.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/lib_warp/boost_uuid_warp.h>
#include <doodle_lib/metadata/project.h>

#include <boost/filesystem.hpp>
#include <magic_enum.hpp>
#include <nlohmann/json_fwd.hpp>
namespace doodle {

enum class department {
  None_,
  Executive,
  Light,
  VFX,
  modle,
  rig,
  Anm,
  direct,
  paint
};

class core_set_init;

/**
 * @brief 全局静态设置类
 *
 * @warning 这个类本身的初始化极为基本和简单， 初始化请使用 core_set_init 分步调用
 */

class DOODLELIB_API core_set : public details::no_copy {
  friend core_set_init;

 public:
  static core_set &getSet();

  //获得运行程序目录
  static FSys::path program_location();
  static FSys::path program_location(const FSys::path &path);

  [[nodiscard]] bool has_maya() const noexcept;
  [[nodiscard]] const FSys::path &maya_path() const noexcept;
  void set_maya_path(const FSys::path &in_MayaPath) noexcept;

  // user设置
  [[nodiscard]] std::string get_user() const;
  [[nodiscard]] std::string get_user_en() const;
  void set_user(const std::string &value);

  //部门设置
  [[nodiscard]] std::string get_department() const;
  [[nodiscard]] const department &get_department_enum() const;
  void set_department(const std::string &value);
  void set_department(const department &value);

  //缓存路径
  [[nodiscard]] FSys::path get_root() const;
  void set_root(const FSys::path &in_root);
  [[nodiscard]] FSys::path get_cache_root() const;
  [[nodiscard]] FSys::path get_cache_root(const FSys::path &in_path) const;
  FSys::path get_data_root() const;

  // doc路径
  [[nodiscard]] FSys::path get_doc() const;
  // 配置文件的路径
  [[nodiscard]] FSys::path get_config_file() const;
  void set_server_host(const string &in_host);

  [[nodiscard]] ue4_setting &get_ue4_setting() const { return p_ue4_setting; };

  [[nodiscard]] int get_sql_port() const;
  void set_sql_port(int in_sqlPort);
  [[nodiscard]] const std::string &get_sql_host() const;
  void set_sql_host(const std::string &in_sqlHost);
  [[nodiscard]] const std::string &get_sql_user() const;
  void set_sql_user(const std::string &in_sqlUser);
  [[nodiscard]] const std::string &get_sql_password() const;
  void set_sql_password(const std::string &in_sqlPassword);
  [[nodiscard]] int get_meta_rpc_port() const;
  void set_meta_rpc_port(int in_metaRpcPort);
  [[nodiscard]] int get_file_rpc_port() const;
  void set_file_rpc_port(int in_fileRpcPort);

  boost::uuids::uuid get_uuid();
  std::string get_uuid_str();

  static std::size_t get_block_size() {
    static std::size_t l_k_i{64 * 1024};
    return l_k_i;
  };

  std::string get_server_host();

  std::uint32_t timeout;
  std::uint16_t p_max_thread;
  void set_max_tread(std::uint16_t in);
  /**
   * @brief 全局是否停止， 服务器使用
   */
  std::atomic_bool p_stop;
  /**
   * @brief 全局锁,服务器使用
   */
  std::mutex p_mutex;
  /**
   * @brief 全局条件变量,服务器使用
   */
  std::condition_variable p_condition;

  std::map<string, bool> widget_show;

  uuid default_project;
  std::array<FSys::path, 10> project_root;
  void add_recent_project(const FSys::path& in);

 private:
  /**
   * @brief 在初始化的时候，我们会进行一些设置，这些设置是及其基本的
   *
   */
  core_set();

  static std::string config_file_name();

 private:
  boost::uuids::random_generator p_uuid_gen;

  //用户名称
  std::string p_user_;
  //部门
  department p_department_;

  FSys::path p_root;
  FSys::path _root_cache;
  FSys::path _root_data;
  FSys::path p_doc;

  ue4_setting &p_ue4_setting;

  FSys::path p_mayaPath;
  std::string p_server_host;  ///< 我们自己的服务器ip

  int p_sql_port;       ///< mysql 端口
  int p_meta_rpc_port;  ///< 元数据端口
  int p_file_rpc_port;  ///< filesys 文件传输端口

  std::string p_sql_host;      ///< mysql数据库ip
  std::string p_sql_user;      ///< mysql 用户名称
  std::string p_sql_password;  ///< mysql 用户密码

  //这里是序列化的代码
  friend void to_json(nlohmann::json &j, const core_set &p);
  friend void from_json(const nlohmann::json &j, core_set &p);
};

void to_json(nlohmann::json &j, const core_set &p);
void from_json(const nlohmann::json &j, core_set &p);
class DOODLELIB_API core_set_init {
  core_set &p_set;

 public:
  core_set_init();

  bool find_maya();
  bool read_file();
  bool write_file();
  bool find_cache_dir();
  bool config_to_user();
  bool init_default_project();
};

namespace win {
/// FOLDERID_Fonts
FSys::path DOODLELIB_API get_font();
FSys::path DOODLELIB_API get_pwd();

}  // namespace win

}  // namespace doodle
