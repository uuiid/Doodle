//
// Created by TD on 2021/5/7.
//

#pragma once
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/metadata/metadata.h>

namespace doodle {
enum class assets_file_type : std::uint32_t {
  none = 0,

};

/**
 * @brief 这个类代表着服务端的文件条目
 *
 */
class DOODLELIB_API assets_file {
 public:
  std::string p_name;
  std::string p_user;
  department p_department;
  std::uint64_t p_version;
  FSys::path path;
  std::string p_ShowName;

 private:
 public:
  /**
   * @brief 默认构造
   *
   */
  assets_file();
  DOODLE_MOVE(assets_file);
  /**
   * @brief 构造一个条目并添加一些必要的属性
   *
   * @param in_metadata 父条目,这个是必须的
   * @param in_path 本地路径,这个在创建时会自动生成一个服务器路径(基本上是一个uuid路径)
   *                基本是调用 AssetsPath::set_path(const FSys::path &in_path, const MetadataConstPtr &in_metadata)
   * @param name 名称
   * @param showName 显示名称
   */
  explicit assets_file(std::string in_show_name);
  // ~AssetsFile();

  [[nodiscard]] std::string str() const;
  [[nodiscard]] std::string show_str() const;

  [[nodiscard]] const std::string& get_user() const;
  void set_user(const std::string& in_user);

  department get_department() const;
  void set_department(department in_department);

  const std::uint64_t& get_version() const noexcept;
  std::string get_version_str() const;
  void set_version(const std::uint64_t& in_Version) noexcept;

  inline void up_version() noexcept {
    set_version(get_version() + 1);
  };

  int find_max_version() const;

  virtual void attribute_widget(const attribute_factory_ptr& in_factoryPtr);

  bool operator<(const assets_file& in_rhs) const;
  bool operator>(const assets_file& in_rhs) const;
  bool operator<=(const assets_file& in_rhs) const;
  bool operator>=(const assets_file& in_rhs) const;

 private:
  friend void to_json(nlohmann::json& j, const assets_file& p) {
    j["name"]       = p.p_name;
    j["ShowName"]   = p.p_ShowName;
    j["user"]       = p.p_user;
    j["department"] = p.p_department;
    j["version"]    = p.p_version;
    j["path"]       = p.path;
  }
  friend void from_json(const nlohmann::json& j, assets_file& p) {
    j.at("name").get_to(p.p_name);
    j.at("ShowName").get_to(p.p_ShowName);
    j.at("user").get_to(p.p_user);
    j.at("department").get_to(p.p_department);
    j.at("version").get_to(p.p_version);
    if (j.contains("path"))
      j.at("path").get_to(p.path);
  }
};

}  // namespace doodle
