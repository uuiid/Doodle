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
  std::string file_namespace;

  void chick_mobject() const;

  /**
   * @brief
   * @warning 这个是一个兼容性函数， 小心使用,会被删除
   * @param in_ref_uuid
   */
  void find_ref_node(const std::string &in_ref_uuid);
  bool find_ref_node();
  bool has_ue4_group() const;
 public:
  MObject p_m_object;
  string path;
  bool use_sim;
  bool high_speed_sim;
  std::vector<string> collision_model;
  std::vector<string> collision_model_show_str;

  reference_file();
  explicit reference_file(const std::string &in_maya_namespace);
  void init_show_name();
  void set_path(const MObject &in_ref_node);
  bool set_namespace(const std::string &in_namespace);

  [[nodiscard]] entt::handle get_prj() const;


  void qlUpdateInitialPose() const;
  [[nodiscard]] MSelectionList get_collision_model() const;
  void set_collision_model(const MSelectionList &in_list);
  [[nodiscard]] string get_namespace() const;

  bool has_node(const MSelectionList &in_list);
  bool has_node(const MObject &in_node) const;

  bool has_sim_cloth();
  /**
   * @brief 没有加载的引用和资产不存在的文件返回false 我们认为这不是异常, 属于正常情况
   */
  bool replace_sim_assets_file();
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
  /**
   * @brief 在这个解算引用中添加一些标记的碰撞
   * @return 返回值只有true , 就算标记碰撞体为空也会返回true 这种情况我们认为正常
   * @throw maya_error maya返回值非成功
   *
   */
  bool add_collision() const;

  bool is_loaded() const;

 private:
  friend void to_json(nlohmann::json &j, const reference_file &p) {
    j["path"]            = p.path;
    j["use_sim"]         = p.use_sim;
    j["high_speed_sim"]  = p.high_speed_sim;
    j["collision_model"] = p.collision_model;
    j["file_namespace"]  = p.file_namespace;
  }
  friend void from_json(const nlohmann::json &j, reference_file &p) {
    j.at("path").get_to(p.path);
    j.at("use_sim").get_to(p.use_sim);
    j.at("high_speed_sim").get_to(p.high_speed_sim);
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
