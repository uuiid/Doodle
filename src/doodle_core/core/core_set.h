#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <boost/process.hpp>

#include <atomic>
#include <cstdint>
#include <string>

namespace doodle {

class user;
class doodle_lib;

/**
 * @brief 全局静态设置类
 *
 * @warning 这个类本身的初始化极为基本和简单
 */

class DOODLE_CORE_API core_set : public boost::noncopyable {
  friend class user;
  friend class doodle_lib;

 public:
  static core_set& get_set();

  void set_root(const FSys::path& in_root);
  [[nodiscard]] FSys::path get_cache_root() const;
  [[nodiscard]] FSys::path get_cache_root(const FSys::path& in_path) const;

  // doc路径
  [[nodiscard]] FSys::path get_doc() const;

  boost::uuids::uuid get_uuid();
  std::string get_uuid_str();

  std::uint32_t timeout;
  std::uint16_t p_max_thread;

  std::locale utf8_locale;

  FSys::path p_root;
  FSys::path p_doc;

  FSys::path ue4_path;
  std::string ue4_version;
  std::int32_t maya_version;

  std::string server_ip{};

  std::string authorize_{};
  void save();
  // 只读模式
  std::atomic_bool read_only_mode_{false};
  // 用户工作根目录
  FSys::path user_work_root_{};

 private:
  // 用户名称
  std::string user_name;
  /**
   * @brief 在初始化的时候，我们会进行一些设置，这些设置是及其基本的
   *
   */
  core_set();

 private:
  FSys::path program_location_attr{};

 private:
  // 这里是序列化的代码
  friend void to_json(nlohmann::json& j, const core_set& p);
  friend void from_json(const nlohmann::json& j, core_set& p);
};

void to_json(nlohmann::json& j, const core_set& p);
void from_json(const nlohmann::json& j, core_set& p);

namespace win {
/// FOLDERID_Fonts
FSys::path DOODLE_CORE_API get_pwd();

}  // namespace win

}  // namespace doodle
