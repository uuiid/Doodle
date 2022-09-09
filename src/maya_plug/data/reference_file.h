//
// Created by TD on 2021/11/30.
//
#pragma once
#include "doodle_lib/doodle_lib_fwd.h"
#include <maya/MSelectionList.h>
namespace doodle::maya_plug {

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

  void chick_mobject() const;

  /**
   * @brief
   * @warning 这个是一个兼容性函数， 小心使用,会被删除
   * @param in_ref_uuid
   */
  void find_ref_node(const std::string &in_ref_uuid);
  bool find_ref_node();
  void bake_results(const MTime &in_start, const MTime &in_end) const;
  /**
   * @brief 导出到abc文件中
   * 这个函数会修改模型和材质名称, 使导出的abc符合ue4导入的标准
   * @param in_start 开始时间
   * @param in_endl 结束时间
   * @param in_export_obj 需要导出的选中列表
   * @return 导出文件的路径
   * @throw maya_error maya返回值非成功
   *
   */
  FSys::path export_abc(
      const MTime &in_start,
      const MTime &in_end,
      const MSelectionList &in_export_obj,
      const std::string &in_abc_name
  ) const;
  /**
   * @brief 导出文件到fbx中, 这个函数会烘培动画帧进行导出
   * @param in_start 开始时间
   * @param in_end 结束时间
   * @param in_export_obj 需要导出的选中列表
   * @return 导出文件的路径
   */
  FSys::path export_fbx(const MTime &in_start, const MTime &in_end, const MSelectionList &in_export_obj) const;

 public:
  /**
   * @brief 引用maya obj 节点
   */
  MObject p_m_object;
  /**
   * @brief 这个路径是显示的路径,  带有后缀以区分相同路径的多个引用
   */
  std::string path;
  /**
   * @brief 引用文件是否解算
   */
  bool use_sim;
  std::vector<std::string> collision_model;
  std::vector<std::string> collision_model_show_str;

  reference_file();
  explicit reference_file(const std::string &in_maya_namespace);
  void init_show_name();
  void set_path(const MObject &in_ref_node);
  bool set_namespace(const std::string &in_namespace);
  /**
   * @brief 将布料初始化状态, (会寻找特点名称的布料进行状态的重置)
   */
  void qlUpdateInitialPose() const;
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
  bool has_sim_cloth();
  /**
   * @brief 没有加载的引用和资产不存在的文件返回false 我们认为这不是异常, 属于正常情况
   */
  bool replace_sim_assets_file();
  /**
   * @brief 替换引用 需要组件 redirection_path_info_edit
   */
  bool replace_file(const entt::handle &in_handle);

  /**
   * @brief 将着色集和材质名称调换为导出abc做准备
   * @return
   */
  bool rename_material() const;
  /**
   * @brief 导出到abc文件中
   * 这个函数会修改模型和材质名称, 使导出的abc符合ue4导入的标准
   * @param in_start 开始时间
   * @param in_endl 结束时间
   * @return 只有在使用maya选择时没有选中物体时返回失败，即推测的导出列表为空时会返回false
   * @throw maya_error maya返回值非成功
   *
   */
  FSys::path export_abc(const MTime &in_start, const MTime &in_end) const;

  FSys::path export_fbx(const MTime &in_start, const MTime &in_end) const;

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

  entt::handle export_file(const export_arg &in_arg);
  entt::handle export_file_select(const export_arg &in_arg, const MSelectionList &in_list);

  /**
   * @brief 在这个解算引用中添加一些标记的碰撞
   * @return 返回值只有true , 就算标记碰撞体为空也会返回true 这种情况我们认为正常
   * @throw maya_error maya返回值非成功
   *
   */
  [[nodiscard]] bool add_collision() const;

  [[nodiscard]] bool is_loaded() const;

  /**
   * @brief 寻找是否有ue4组(作为导出标志)
   * @return
   */
  bool has_ue4_group() const;

  /**
   * @brief 从配置文件中查找需要导出组名称对应的 maya 组 (名称空间为引用空间)
   * @return 导出配置文件中对应的组
   */
  std::optional<MDagPath> export_group_attr() const;

  /**
   * @brief 获取所有需要导出布料所包裹的高模,
   * @warning 必须使用maya 节点连接到导出模型, 从解算节点开始查找(qcloth或者ncloth)
   * @return 所有需要导出的路径
   */
  std::vector<MDagPath> qcloth_export_model() const;

  /**
   * @brief 获取这个抽象引用中所有的 obj
   * @return 选中所有的obj
   */
  MSelectionList get_all_object() const;

 private:
  friend void to_json(nlohmann::json &j, const reference_file &p) {
    j["path"]            = p.path;
    j["use_sim"]         = p.use_sim;
    j["collision_model"] = p.collision_model;
    j["file_namespace"]  = p.file_namespace;
  }
  friend void from_json(const nlohmann::json &j, reference_file &p) {
    j.at("path").get_to(p.path);
    j.at("use_sim").get_to(p.use_sim);
    j.at("collision_model").get_to(p.collision_model);
    if (j.contains("file_namespace"))
      j.at("file_namespace").get_to(p.file_namespace);

    if (j.contains("ref_file_uuid")) {
      std::string ref_file_uuid;
      j.at("ref_file_uuid").get_to(ref_file_uuid);
      p.find_ref_node(ref_file_uuid);
    }
    p.chick_mobject();
  }
};

}  // namespace doodle::maya_plug
