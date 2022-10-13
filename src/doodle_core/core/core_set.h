#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <boost/process.hpp>
namespace doodle {

class core_set_init;
class user;
class doodle_lib;

/**
 * @brief 全局静态设置类
 *
 * @warning 这个类本身的初始化极为基本和简单， 初始化请使用 core_set_init 分步调用
 */

class DOODLE_CORE_API core_set : public details::no_copy {
  friend class core_set_init;
  friend class user;
  friend class doodle_lib;

 public:
  static core_set &get_set();

  // 获得运行程序目录
  FSys::path program_location();

  [[nodiscard]] bool has_maya() const noexcept;
  [[nodiscard]] const FSys::path &maya_path() const noexcept;
  void set_maya_path(const FSys::path &in_MayaPath) noexcept;

  void set_root(const FSys::path &in_root);
  [[nodiscard]] FSys::path get_cache_root() const;
  [[nodiscard]] FSys::path get_cache_root(const FSys::path &in_path) const;
  [[nodiscard]] FSys::path get_data_root() const;

  // doc路径
  [[nodiscard]] FSys::path get_doc() const;

  boost::uuids::uuid get_uuid();
  std::string get_uuid_str();
  std::string get_uuid_str(const std::string &in_add);

  std::uint32_t timeout;
  std::uint16_t p_max_thread;

  std::array<FSys::path, 10> project_root;
  void add_recent_project(const FSys::path &in);

  // 部门
  std::string organization_name;

  std::shared_ptr<nlohmann::json> json_data;

  FSys::path p_root;
  FSys::path _root_cache;
  FSys::path _root_data;
  FSys::path p_doc;

  FSys::path p_mayaPath;
  FSys::path ue4_path;
  std::string ue4_version;
  bool maya_replace_save_dialog{false};
  bool maya_force_resolve_link{false};

  doodle_lib *lib_ptr{};
  logger_ctrl *log_ptr{};

 private:
  // 用户名称
  boost::uuids::uuid user_id;
  std::string user_name;
  /**
   * @brief 在初始化的时候，我们会进行一些设置，这些设置是及其基本的
   *
   */
  core_set();

  static std::string config_file_name();

 private:
  boost::uuids::random_generator p_uuid_gen;
  FSys::path program_location_attr{};

 private:
  // 这里是序列化的代码
  friend void to_json(nlohmann::json &j, const core_set &p);
  friend void from_json(const nlohmann::json &j, core_set &p);
};

void to_json(nlohmann::json &j, const core_set &p);
void from_json(const nlohmann::json &j, core_set &p);
class DOODLE_CORE_API core_set_init {
  core_set &p_set;

 public:
  core_set_init();

  bool find_maya();
  void read_file();
  bool write_file();
  bool config_to_user();
};

namespace win {
/// FOLDERID_Fonts
FSys::path DOODLE_CORE_API get_font();
FSys::path DOODLE_CORE_API get_pwd();

}  // namespace win

}  // namespace doodle
