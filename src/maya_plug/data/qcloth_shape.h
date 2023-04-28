//
// Created by TD on 2021/12/6.
//

#pragma once
#include <maya_plug/data/cloth_interface.h>
#include <maya_plug/main/maya_plug_fwd.h>

#include "entt/entity/fwd.hpp"
#include "maya/MApiNamespace.h"
#include <maya/MObject.h>

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

  static cloth_group get_cloth_group();

  /**
   * @brief 获取传入动画节点(动画[绑定]网格体或者变换节点)链接的皮肤簇
   * @param in_anim_node 动画[绑定]网格体或者变换节点
   * @return 寻找到的皮肤簇(不为空)
   * @throw 为空时抛出异常 maya_error
   */
  static MObject get_skin_custer(const MObject& in_anim_node);
  /**
   * @brief 重置传入动画节点(动画[绑定]网格体或者变换节点)链接的皮肤簇属性
   * @param in_anim_node 动画[绑定]网格体或者变换节点
   */
  static void rest_skin_custer_attr(const MObject& in_anim_node);

 private:
  mutable entt::handle p_ref_file;

  entt::handle_view<reference_file> find_ref_file() const;

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
  void clear_cache() const override;

  /**
   * @brief 设置qcloth缓存路径,如果存在缓存文件,还会删除缓存文件
   * @return 完成设置
   */
  void set_cache_folder(const entt::handle& in_handle, const FSys::path& in_path) const override;
  /**
   * @brief 使用 MPlug::asMObject 作为强行评估节点属性的方法, 在
   * 没有gui的情况下包装解算的正常
   *
   *
   * @throw maya_error 有可能找不到需要评估的属性, 抛出异常
   * @throw doodle_error 也可能属性 mobj 为空, 抛出异常
   * @return true MObject 不空
   * @return false 空mobj(有可能没有完成评估)
   */
  bool create_cache() const;

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
   * @brief 从传入的实体创建一个绑定节点
   * @param in_handle 传入的一个实体,
   * 必须具备 qcloth_shape_n::maya_mesh, qcloth_shape_n::high_shape_list组件
   * 可选的具备 qcloth_shape_n::collision_shape_list组件
   *
   *
   * @note
   * * 创建一个空的mesh 节点作为绑定动画的输出;（将动画 outMesh 链接到 inMesh ） \n
   * * 从新的的网格体创建布料 \n
   * * 创建一个高模的复制体, 将低模和高模进行包裹变形 \n
   * * 将复制出来的高模物体链接到绑定文件中（这个以后做  中间需要插入一个切换表达式节点用来切换动画和解算） \n
   *
   *  需要读取配置文件中的各个属性, 进行标准的重命名
   */
  static std::vector<entt::handle> create_sim_cloth(const entt::handle& in_handle);

  /**
   * @brief 重置maya高模皮肤簇节点为权重为1
   * @param in_handle 传入的句柄 需要具备 qcloth_shape_n::shape_list 组件
   */
  static void reset_create_node_attribute(const entt::handle& in_handle);
  /**
   * @brief 检查所有传入的简模是否进行了蒙皮
   * @param in_handle 传入的一个实体,
   * 必须具备 qcloth_shape_n::maya_mesh
   * @return 具有蒙皮
   */
  static bool chick_low_skin(const entt::handle& in_handle);

  static void add_collider(const entt::handle& in_handle);
  static void sort_group();

  /**
   * @brief 根据引用文件创建布料句柄
   * @param in_ref_file 传入的引用文件句柄
   * @return 完成布料网格的创建的句柄
   */

  static MObject get_ql_solver(const MSelectionList& in_selection_list);
  static MObject get_ql_solver();
};

}  // namespace doodle::maya_plug
