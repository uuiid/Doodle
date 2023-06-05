//
// Created by TD on 2021/11/30.
//
#pragma once
#include <maya_plug/main/maya_plug_fwd.h>

#include "entt/entity/fwd.hpp"
#include "maya/MString.h"
#include <maya/MSelectionList.h>
#include <maya/MTime.h>
#include <vector>
namespace doodle::maya_plug {
class reference_file;
namespace reference_file_ns {
class generate_file_path_base : boost::less_than_comparable<generate_file_path_base> {
 protected:
  std::string extract_reference_name;
  std::string format_reference_name;
  std::string extract_scene_name;
  std::string format_scene_name;
  bool use_add_range;
  template <typename T1, typename Char, typename Enable>
  friend struct fmt::formatter;

  virtual FSys::path get_path() const                               = 0;
  virtual FSys::path get_name(const std::string &in_ref_name) const = 0;

  std::string get_extract_scene_name(const std::string &in_name) const;
  std::string get_extract_reference_name(const std::string &in_name) const;

 public:
  generate_file_path_base()          = default;
  virtual ~generate_file_path_base() = default;

  std::optional<std::string> add_external_string;

  std::pair<MTime, MTime> begin_end_time;
  FSys::path operator()(const reference_file &in_ref) const;
  [[nodiscard("")]] bool operator==(const generate_file_path_base &in) const noexcept;
  [[nodiscard("")]] bool operator<(const generate_file_path_base &in) const noexcept;
};

class generate_abc_file_path : boost::less_than_comparable<generate_abc_file_path>, public generate_file_path_base {
 protected:
  [[nodiscard("")]] FSys::path get_path() const override;
  [[nodiscard("")]] FSys::path get_name(const std::string &in_ref_name) const override;
  friend struct fmt::formatter<generate_file_path_base>;

 public:
  explicit generate_abc_file_path(const entt::registry &in = *g_reg());
  virtual ~generate_abc_file_path();
};

class generate_fbx_file_path : boost::less_than_comparable<generate_fbx_file_path>, public generate_file_path_base {
  friend struct fmt::formatter<generate_fbx_file_path>;

 private:
  std::string camera_suffix{};
  bool is_camera_attr{};

 protected:
  [[nodiscard("")]] FSys::path get_path() const override;
  [[nodiscard("")]] FSys::path get_name(const std::string &in_ref_name) const override;

 public:
  explicit generate_fbx_file_path(const entt::registry &in = *g_reg());

  void is_camera(bool in_is_camera);

  virtual ~generate_fbx_file_path();
};

}  // namespace reference_file_ns

/**
 * @brief 一个类似的引用文件使用名称空间作为引用的定位,
 * 而不是ref节点,
 * 这样我们可以在文件中创建出一个类似的引用, 但不是引用,
 * 并且具有一定引用概念的类
 */
class reference_file {
 public:
 private:
  std::string file_namespace;

  entt::handle search_file_info;
  /// @brief 添加风场字段
  std::string field_attr;

  void chick_mobject() const;

  static std::string get_abc_exprt_arg();

  /**
   * @brief
   * @warning 这个是一个兼容性函数， 小心使用,会被删除
   * @param in_ref_uuid
   */
  void find_ref_node(const std::string &in_ref_uuid);
  bool find_ref_node();

  bool has_chick_group() const;

  std::vector<MObject> ref_objs{};
  /**
   * @brief 这个路径是显示的路径,  带有后缀以区分相同路径的多个引用
   */
  std::string path;

 public:
  /**
   * @brief 引用maya obj 节点
   */
  MObject p_m_object;

  /**
   * @brief 引用文件是否解算
   */
  bool use_sim;
  std::vector<std::string> collision_model;
  std::vector<std::string> collision_model_show_str;

  reference_file();
  explicit reference_file(const std::string &in_maya_namespace);
  explicit reference_file(const MString &in_maya) : reference_file(std::string{in_maya.asUTF8()}) {}
  void init_show_name();
  void set_path(const MObject &in_ref_node);
  bool set_namespace(const std::string &in_namespace);
  /// 获取引用标帜路径
  const std::string &get_key_path() const;

  [[nodiscard]] MSelectionList get_collision_model() const;
  void set_collision_model(const MSelectionList &in_list);
  [[nodiscard]] std::string get_namespace() const;

  [[nodiscard]] bool has_node(const MSelectionList &in_list);
  [[nodiscard]] bool has_node(const MObject &in_node) const;
  /**
   * @brief 获取真正的路径名称
   * @return
   */
  [[nodiscard]] FSys::path get_path() const;
  /**
   * @brief 获取真正的路径名称
   * @return
   */
  [[nodiscard]] FSys::path get_abs_path() const;
  /**
   * @brief 没有加载的引用和资产不存在的文件返回false 我们认为这不是异常, 属于正常情况
   */
  bool replace_sim_assets_file();
  /**
   * @brief 替换引用 需要组件 redirection_path_info_edit
   */
  bool replace_file(const entt::handle &in_handle);

  std::optional<MDagPath> get_field_dag() const;
  const std::string &get_field_string() const;
  void add_field_dag(const MSelectionList &in_list);

  enum class export_type : std::uint32_t {
    abc = 1,
    fbx = 2,
  };

  class export_arg {
   public:
    export_type export_type_p{};
    MTime &start_p;
    MTime &end_p;
  };

  /**
   * @brief 从配置文件中查找需要导出组名称对应的 maya 组 (名称空间为引用空间)
   * @return 导出配置文件中对应的组
   */
  std::optional<MDagPath> export_group_attr() const;

  /**
   * @brief 获取这个抽象引用中所有的 obj
   * @return 选中所有的obj
   */
  MSelectionList get_all_object() const;

  std::vector<MDagPath> get_alll_cloth_obj() const;

  explicit inline operator bool() const { return has_chick_group(); }

 private:
  friend void to_json(nlohmann::json &j, const reference_file &p) {
    j["path"]            = p.path;
    j["use_sim"]         = p.use_sim;
    j["collision_model"] = p.collision_model;
    j["file_namespace"]  = p.file_namespace;
    j["field_attr"]      = p.field_attr;
  }
  friend void from_json(const nlohmann::json &j, reference_file &p) {
    j.at("path").get_to(p.path);
    j.at("use_sim").get_to(p.use_sim);
    j.at("collision_model").get_to(p.collision_model);
    if (j.contains("file_namespace")) j.at("file_namespace").get_to(p.file_namespace);
    if (j.contains("field_attr")) j.at("field_attr").get_to(p.field_attr);

    if (j.contains("ref_file_uuid")) {
      std::string ref_file_uuid;
      j.at("ref_file_uuid").get_to(ref_file_uuid);
      p.find_ref_node(ref_file_uuid);
    }
    p.chick_mobject();
  }
};

class reference_file_factory {
 public:
  reference_file_factory()  = default;
  ~reference_file_factory() = default;

  [[nodiscard]] std::vector<entt::handle> create_ref() const;
};

}  // namespace doodle::maya_plug

namespace fmt {
/**
 * @brief
 *
 * @tparam
 */
template <>
struct formatter< ::doodle::maya_plug::reference_file_ns::generate_abc_file_path> : formatter<std::string> {
  template <typename FormatContext>
  auto format(const ::doodle::maya_plug::reference_file_ns::generate_abc_file_path &in_, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    return fmt::format_to(
        ctx.out(), "extract_scene_name : {} extract_reference_name : {} use_add_range : {} add_external_string : {}",
        in_.extract_scene_name, in_.extract_reference_name, in_.use_add_range,
        in_.add_external_string ? *in_.add_external_string : std::string{}
    );
  }
};
}  // namespace fmt
