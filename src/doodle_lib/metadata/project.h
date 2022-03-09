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
};

class DOODLELIB_API model_config {
 public:
  model_config();
  /**
   * @brief 寻找拖入文件时的图标的正则表达式
   */
  std::string find_icon_regex;

  friend void to_json(nlohmann::json& j, const model_config& p) {
    j["find_icon_regex"] = p.find_icon_regex;
  }
  friend void from_json(const nlohmann::json& j, model_config& p) {
    j.at("find_icon_regex").get_to(p.find_icon_regex);
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
   * @brief 计算maya qcloth 布料中 初始化位置时 绑定皮肤簇网格 优先级
   */
  std::vector<std::pair<std::string, std::int32_t>> skin_priority_list;

  /**
   * @brief 计算maya qcloth 布料中 qcloth布料 优先级
   */
  std::vector<std::pair<std::string, std::int32_t>> cloth_priority_list;

  /**
   * @brief 获取 skin_priority_list ，当为空时， 填充返回默认值
   * @return skin_priority_list
   */
  std::vector<std::pair<std::string, std::int32_t>> get_skin_priority_list() const;
  /**
   * @brief 获取 cloth_priority_list ，当为空时， 填充返回默认值
   * @return cloth_priority_list
   */
  std::vector<std::pair<std::string, std::int32_t>> get_cloth_priority_list() const;

  friend void to_json(nlohmann::json& j, const cloth_config& p) {
    j["vfx_cloth_sim_path"]  = p.vfx_cloth_sim_path;
    j["export_group"]        = p.export_group;
    j["skin_priority_list"]  = p.skin_priority_list;
    j["cloth_priority_list"] = p.cloth_priority_list;
  }
  friend void from_json(const nlohmann::json& j, cloth_config& p) {
    j.at("vfx_cloth_sim_path").get_to(p.vfx_cloth_sim_path);
    j.at("export_group").get_to(p.export_group);
    if (j.contains("skin_priority_list"))
      j.at("skin_priority_list").get_to(p.skin_priority_list);
    if (j.contains("cloth_priority_list"))
      j.at("cloth_priority_list").get_to(p.cloth_priority_list);
  }
  constexpr static std::uint32_t class_hash() {
    return "class doodle::project::cloth_config"_hs;
  }
};
}  // namespace project_config
}  // namespace doodle
