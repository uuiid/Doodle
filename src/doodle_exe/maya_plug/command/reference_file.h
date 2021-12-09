//
// Created by TD on 2021/11/30.
//
#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <maya/MSelectionList.h>
namespace doodle::maya_plug {

class reference_file {
  uuid prj_ref;
  string ref_file_uuid;
  std::vector<entt::handle> p_cloth_shape;
  MObject p_m_object;

  void chick_mobject();
  void chick_mobject() const;

 public:
  string path;
  bool use_sim;
  bool high_speed_sim;
  std::vector<string> collision_model;
  std::vector<string> collision_model_show_str;

  reference_file();
  explicit reference_file(const entt::handle &in_uuid, const MObject &in_ref_node);

  void init_show_name();
  void set_path(const MObject &in_ref_node);

  [[nodiscard]] entt::handle get_prj() const;

  [[nodiscard]] MSelectionList get_collision_model() const;
  void set_collision_model(const MSelectionList &in_list);
  [[nodiscard]] string get_namespace() const;
  [[nodiscard]] string get_namespace();
  [[nodiscard]] string get_unique_name() const;
  /**
   * @brief 没有加载的引用和资产不存在的文件返回false 我们认为这不是异常, 属于正常情况
   */
  bool replace_sim_assets_file();
  /**
   * @brief 创建当前帧的缓存
   * @return
   */
  bool create_cache() const;
  /**
   * @brief 将着色集和材质名称调换为导出abc做准备
   * @return
   */
  bool rename_material() const;
  bool export_abc(const MTime &in_start, const MTime &in_endl) const;
  bool add_collision() const;

 private:
  friend void to_json(nlohmann::json &j, const reference_file &p) {
    j["path"]            = p.path;
    j["use_sim"]         = p.use_sim;
    j["high_speed_sim"]  = p.high_speed_sim;
    j["collision_model"] = p.collision_model;
    j["prj_ref"]         = p.prj_ref;
    j["ref_file_uuid"]   = p.ref_file_uuid;
  }
  friend void from_json(const nlohmann::json &j, reference_file &p) {
    j.at("path").get_to(p.path);
    j.at("use_sim").get_to(p.use_sim);
    j.at("prj_ref").get_to(p.prj_ref);
    j.at("high_speed_sim").get_to(p.high_speed_sim);
    j.at("collision_model").get_to(p.collision_model);
    j.at("ref_file_uuid").get_to(p.ref_file_uuid);
    p.chick_mobject();
  }
};

}  // namespace doodle::maya_plug
