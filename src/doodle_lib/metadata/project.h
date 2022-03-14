#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
namespace project_config {
class base_config;
class model_config;
class cloth_config;
}  // namespace project_config

/**
 * 项目信息类
 */
class DOODLELIB_API project {
 public:
  using cloth_config = project_config::cloth_config;

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
namespace project_config {
class DOODLELIB_API base_config {
 public:
  [[nodiscard]] static std::string get_current_find_icon_regex_();
  [[nodiscard]] static std::vector<std::string> get_assets_paths();
};

class DOODLELIB_API model_config {
 public:
  model_config();
  /**
   * @brief 寻找拖入文件时的图标的正则表达式
   */
  std::string find_icon_regex;

  /**
   * @brief 本组的各种分类
   *
   */
  std::vector<std::string> assets_list;

  friend void to_json(nlohmann::json& j, const model_config& p) {
    j["find_icon_regex"] = p.find_icon_regex;
    j["assets_list"]     = p.assets_list;
  }
  friend void from_json(const nlohmann::json& j, model_config& p) {
    j.at("find_icon_regex").get_to(p.find_icon_regex);
    if (j.contains("assets_list"))
      j.at("assets_list").get_to(p.assets_list);
  }
};

class DOODLELIB_API cloth_config {
 public:
  cloth_config();
  FSys::path vfx_cloth_sim_path;

  /**
   * @brief 导出整个解算文件所需要的选择组
   * 我们使用这个组导出整个解算结果
   */
  std::string export_group;
  /**
   * @brief 导出整个解算文件所需要的选择组
   * 我们使用这个组导出整个解算结果
   */
  std::string cloth_proxy_;
  /**
   * @brief 导出整个解算文件所需要的选择组
   * 我们使用这个组导出整个解算结果
   */
  std::string simple_module_proxy_;

  friend void to_json(nlohmann::json& j, const cloth_config& p) {
    j["vfx_cloth_sim_path"]   = p.vfx_cloth_sim_path;
    j["export_group"]         = p.export_group;
    j["cloth_proxy_"]         = p.cloth_proxy_;
    j["simple_module_proxy_"] = p.simple_module_proxy_;
  }
  friend void from_json(const nlohmann::json& j, cloth_config& p) {
    j.at("vfx_cloth_sim_path").get_to(p.vfx_cloth_sim_path);
    j.at("export_group").get_to(p.export_group);
    if (j.contains("cloth_proxy_"))
      j.at("cloth_proxy_").get_to(p.cloth_proxy_);
    if (j.contains("simple_module_proxy_"))
      j.at("simple_module_proxy_").get_to(p.simple_module_proxy_);
  }
  constexpr static std::uint32_t class_hash() {
    return "class doodle::project::cloth_config"_hs;
  }
};
}  // namespace project_config
}  // namespace doodle
