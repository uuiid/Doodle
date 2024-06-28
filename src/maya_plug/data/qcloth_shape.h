//
// Created by TD on 2021/12/6.
//

#pragma once
#include <maya_plug/data/cloth_interface.h>
#include <maya_plug/main/maya_plug_fwd.h>

#include "entt/entity/fwd.hpp"
#include "maya/MApiNamespace.h"
#include <maya/MObject.h>
#include <string>

// #include <maya/MDagPath.h>

namespace doodle::maya_plug {
class reference_file;
namespace qcloth_shape_n {
class maya_obj;
}  // namespace qcloth_shape_n

namespace qcloth_shape_n {
/**
 * @brief 这个是个maya 节点 obj的小型包装类, 构造函数会提取节点的名称, 可以用来显示
 */
class maya_obj {
 public:
  maya_obj();
  explicit maya_obj(const MObject& in_object);
  MObject obj;
  std::string p_name;
};

/**
 * @brief 一个maya obj列表
 */
using shape_list = std::vector<maya_obj>;

}  // namespace qcloth_shape_n

class qcloth_shape : public cloth_interface::element_type {
 public:
  class cloth_group {
   public:
    MObject cfx_grp;
    MObject solver_grp;
    MObject anim_grp;
    MObject constraint_grp;
    MObject collider_grp;
    MObject deform_grp;
    MObject export_grp;
    MObject deformBase_grp;
  };

  /**
   * @brief 获取传入动画节点(动画[绑定]网格体或者变换节点)链接的皮肤簇
   * @param in_anim_node 动画[绑定]网格体或者变换节点
   * @return 寻找到的皮肤簇(不为空)
   * @throw 为空时抛出异常 maya_error
   */
  static MObject get_skin_custer(const MObject& in_anim_node);

 private:
  /**
   * @brief qlClothShape 类型 节点
   */
  MObject obj;

 public:
  inline static MString qlSolverShape{L"qlSolverShape"};
  inline static MString qlClothShape{L"qlClothShape"};
  qcloth_shape();

  /**
   *
   * @param in_object qcloth shape object
   */
  explicit qcloth_shape(const MObject& in_object);

  void sim_cloth() const override;
  void add_field(const entt::handle& in_handle) const override;
  void add_collision(const entt::handle& in_handle) const override;
  void rest(const entt::handle& in_handle) const override;
  [[nodiscard]] MObject get_solver() const override;
  void set_cache_folder(const entt::handle& in_handle, const FSys::path& in_path, bool need_clear) const override;
  [[nodiscard]] std::string get_namespace() const override;
  void cover_cloth_attr(const entt::handle& in_handle) const override;
  [[nodiscard]] MDagPath get_shape() const override;

  /**
   * @brief 获取布料形状（这个是一个tran）
   * @return 布料dag路径
   */
  [[nodiscard]] MDagPath ql_cloth_shape() const;
  /**
   * @brief 获取布料输出的 mesh 节点，
   * @return 布料网格dag
   */
  [[nodiscard]] MDagPath cloth_mesh() const;

  /**
   * @brief 检查所有传入的简模是否进行了蒙皮
   * @param in_handle 传入的一个实体,
   * 必须具备 qcloth_shape_n::maya_mesh
   * @return 具有蒙皮
   */
  static bool chick_low_skin(const entt::handle& in_handle);

  /**
   * @brief 根据引用文件创建布料句柄
   * @return 完成布料网格的创建的句柄
   */

  static MObject get_ql_solver(const MSelectionList& in_selection_list);
  static MObject get_ql_solver();
};

}  // namespace doodle::maya_plug
