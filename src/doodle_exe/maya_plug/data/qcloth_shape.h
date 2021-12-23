//
// Created by TD on 2021/12/6.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <maya/MObject.h>
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
  string p_name;
};

/**
 * @brief 一个maya obj列表
 */
using shape_list = std::vector<maya_obj>;

}  // namespace qcloth_shape_n

class qcloth_shape {
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
  };

  static cloth_group get_cloth_group();

 private:
  entt::handle p_ref_file;
  /**
   * @brief qlClothShape 类型 节点
   */
  MObject obj;


 public:
  qcloth_shape();
  explicit qcloth_shape(const entt::handle& in_ref_file, const MObject& in_object);

  /**
   * @brief 设置qcloth缓存路径,如果存在缓存文件,还会删除缓存文件
   * @return 完成设置
   */
  bool set_cache_folder() const;
  bool create_cache() const;
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
  void create_sim_cloth(const entt::handle& in_handle);

  static void set_all_active(bool in_active);
  static void set_all_attraction_method(bool in_);
};
}  // namespace doodle::maya_plug
