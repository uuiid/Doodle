#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <rttr/rttr_enable.h>

namespace doodle {
namespace project_config {
class base_config;

}  // namespace project_config

/**
 * 项目信息类
 */
class DOODLE_CORE_API project {
 public:
  std::string p_name;
  FSys::path p_path;

  std::string p_en_str;
  std::string p_shor_str;
  RTTR_ENABLE();

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
void DOODLE_CORE_API to_json(nlohmann::json& j, const base_config& p);
void DOODLE_CORE_API from_json(const nlohmann::json& j, base_config& p);

using camera_judge = std::pair<std::string, std::int32_t>;

class DOODLE_CORE_API base_config {
 private:
  friend void DOODLE_CORE_API to_json(nlohmann::json& j, const base_config& p);
  friend void DOODLE_CORE_API from_json(const nlohmann::json& j, base_config& p);
  RTTR_ENABLE();

 public:
  constexpr static std::uint32_t class_hash() { return "class doodle::project::cloth_config"_hs; }

 public:
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

  /**
   * @brief 寻找拖入文件时的图标的正则表达式
   */
  std::string find_icon_regex;

  /**
   * @brief 本组的各种分类
   *
   */
  std::vector<std::string> assets_list;

  /**
   * @brief 本组的各种分类
   *
   */
  std::vector<std::string> icon_extensions;
  /// \brief 导出后上传路径
  FSys::path upload_path;
  /// \brief 季数包含的集数个数
  std::int32_t season_count;

  /// 先判断是否只导出解算体, 然后判断是否划分网格
  /// \brief 是否只导出解算物体
  bool use_only_sim_cloth{false};
  /// \brief 是否进行分组导出
  bool use_divide_group_export{false};
  /// 重命名和合并网格体是在最后判断的

  /// \brief maya导出 abc 时是否进行作色集和材质名称进行调换
  bool use_rename_material{true};
  /// \brief maya导出时, 是否进行合并网格操作
  bool use_merge_mesh{true};

  /// \brief t post 时间
  std::int32_t t_post{950u};
  /// \brief 导出动画时间
  std::int32_t export_anim_time{1001u};

  /// \brief 使用camera优先级寻找maya 相机
  std::vector<camera_judge> maya_camera_select{};
  /// \brief 是否导出自定义元数据
  bool use_write_metadata{true};

  /// \brief 导出时重新提取引用名称
  std::string abc_export_extract_reference_name{};
  std::string abc_export_format_reference_name{};

  /// \brief 导出时重新提取场景名称
  std::string abc_export_extract_scene_name{};
  std::string abc_export_format_scene_name{};

  /// \brief 添加帧范围后缀
  bool abc_export_add_frame_range{true};
  /// \brief camera 文件名称后缀
  std::string maya_camera_suffix;
  /// @brief 输出abc时的后缀
  std::string maya_out_put_abc_suffix;

  base_config();

  [[nodiscard]] bool match_icon_extensions(const FSys::path& in_path) const;

  [[nodiscard]] FSys::path get_upload_path() const;
};

}  // namespace project_config
}  // namespace doodle
