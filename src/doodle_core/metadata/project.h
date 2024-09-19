#pragma once
#include <doodle_core/core/core_set.h>
#include <doodle_core/doodle_core_fwd.h>
namespace doodle {
namespace project_config {
class base_config;

}  // namespace project_config

/**
 * 项目信息类
 */
class DOODLE_CORE_API project : boost::totally_ordered<project> {
 public:
  // 项目名称
  std::string p_name;
  // 项目路径
  FSys::path p_path;
  // 项目全拼
  std::string p_en_str;
  // 项目简拼
  std::string p_shor_str;
  // 项目本机路径
  FSys::path p_local_path;
  // 自动灯光上传路径
  FSys::path p_auto_upload_path;

 private:
  void init_name();

 public:
  /// 添加entt指示, 用于保持指针稳定性
  static constexpr bool in_place_delete{true};
  project();
  explicit project(FSys::path in_path, std::string in_name);
  explicit project(
      std::string in_name, FSys::path in_path, std::string in_en_str, std::string in_shor_str, FSys::path in_local_path,
      FSys::path in_auto_upload_path
  )
      : p_name(std::move(in_name)),
        p_path(std::move(in_path)),
        p_en_str(std::move(in_en_str)),
        p_shor_str(std::move(in_shor_str)),
        p_local_path(std::move(in_local_path)),
        p_auto_upload_path(std::move(in_auto_upload_path)) {}

  [[nodiscard]] const std::string& get_name() const;
  void set_name(const std::string& Name) noexcept;

  [[nodiscard]] const FSys::path& get_path() const noexcept;
  [[nodiscard]] FSys::path make_path(const FSys::path& in_path) const;
  void set_path(const FSys::path& Path);

  [[nodiscard]] std::string str() const;
  [[nodiscard]] std::string show_str() const;

  [[nodiscard]] std::string short_str() const;

  bool operator<(const project& in_rhs) const;
  bool operator==(const project& in_rhs) const;

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

struct project_ptr {
  project* project_;
};

class project_helper {
  static void dependent_uuid(entt::registry& in_reg, entt::entity in_entity);
  static void on_destroy(entt::registry& in_reg, entt::entity in_entity);

 public:

  struct project_ctx_t {
    std::array<entt::scoped_connection, 3> conn_;
  };

  struct database_t {
    std::int32_t id_{};
    uuid uuid_id_{};

    std::string name_{};
    std::string path_{};
    std::string en_str_{};
    std::string shor_str_{};
    std::string local_path_{};
    std::string auto_upload_path_{};
  };

  static std::vector<entt::entity> load_from_sql(entt::registry& reg, const std::vector<database_t>& in_data);
  static void seed_to_sql(const entt::registry& in_registry, const std::vector<entt::entity>& in_entity);
};

namespace project_config {
void DOODLE_CORE_API to_json(nlohmann::json& j, const base_config& p);
void DOODLE_CORE_API from_json(const nlohmann::json& j, base_config& p);

using camera_judge = std::pair<std::string, std::int32_t>;

class DOODLE_CORE_API base_config {
 private:
  friend void DOODLE_CORE_API to_json(nlohmann::json& j, const base_config& p);
  friend void DOODLE_CORE_API from_json(const nlohmann::json& j, base_config& p);

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

  /// \brief t post 时间
  std::int32_t t_post{950u};
  /// \brief 导出动画时间
  std::int32_t export_anim_time{1001u};

  /// \brief 使用camera优先级寻找maya 相机
  std::vector<camera_judge> maya_camera_select{};

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

  base_config() = default;

  [[nodiscard]] bool match_icon_extensions(const FSys::path& in_path) const;

  [[nodiscard]] FSys::path get_upload_path() const;

  static base_config get_default();
};

}  // namespace project_config
}  // namespace doodle
