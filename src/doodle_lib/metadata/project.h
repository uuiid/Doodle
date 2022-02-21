#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
/**
 * 项目信息类
 */
class DOODLELIB_API project {
 public:
  class DOODLELIB_API cloth_config {
   public:
    cloth_config();
    FSys::path vfx_cloth_sim_path;

    /**
     * @brief 导出整个解算文件所需要的选择组
     * 我们使用这个组导出整个解算结果
     */
    std::string export_group;

    friend void to_json(nlohmann::json& j, const cloth_config& p) {
      j["vfx_cloth_sim_path"] = p.vfx_cloth_sim_path;
      j["export_group"]       = p.export_group;
    }
    friend void from_json(const nlohmann::json& j, cloth_config& p) {
      j.at("vfx_cloth_sim_path").get_to(p.vfx_cloth_sim_path);
      j.at("export_group").get_to(p.export_group);
    }
  };

  std::string p_name;
  FSys::path p_path;

  std::string p_en_str;
  std::string p_shor_str;
 private:
  void init_name();

 public:
  project();
  explicit project(FSys::path in_path, std::string in_name);

  [[nodiscard]] const std::string& get_name() const;
  void set_name(const std::string& Name) noexcept;

  [[nodiscard]] const FSys::path& get_path() const noexcept;
  [[nodiscard]] FSys::path make_path(const FSys::path& in_path) const;
  void set_path(const FSys::path& Path);

  [[nodiscard]] std::string str() const;
  [[nodiscard]] std::string show_str() const;

  [[nodiscard]] std::string short_str() const;

  static entt::handle get_current();
 
  bool operator<(const project& in_rhs) const;
  bool operator>(const project& in_rhs) const;
  bool operator<=(const project& in_rhs) const;
  bool operator>=(const project& in_rhs) const;
  bool operator==(const project& in_rhs) const;
  bool operator!=(const project& in_rhs) const;

 private:
  friend void to_json(nlohmann::json& j, const project& p) {
    j["name"] = p.p_name;
    j["path"] = p.p_path;
  }
  friend void from_json(const nlohmann::json& j, project& p) {
    j.at("name").get_to(p.p_name);
    j.at("path").get_to(p.p_path);
    p.init_name();
  }
};

}  // namespace doodle
