//
// Created by TD on 2022/3/4.
//
#pragma once

#include <maya_plug/main/maya_plug_fwd.h>
namespace doodle::maya_plug {
/**
 * @brief 获取maya中的多边形的信息类, 在初始化中就会获取
 */
class maya_poly_info {
 private:
 public:
  /// \brief 获得的maya obj引用
  MObject maya_obj;
  /// \brief 是否在上游查找到了皮肤簇节点
  bool has_skin;
  /// \brief 是否是中间物体
  bool is_intermediate_obj;
  /// \brief 使用可以在下游查找到ql布料解算节点
  bool has_cloth;

  std::string node_name;
  std::string node_org_name;
  //  std::int32_t numColors;
  //  std::int32_t numColorSets;
  /**
   * @brief 默认初始化构造
   */
  maya_poly_info();
  /**
   * @brief 直接初始化构造
   * @param in_mesh_object 传入的 maya obj 用来查找属性的节点指针
   */
  explicit maya_poly_info(const MObject& in_mesh_object);
  /**
   * @brief 比较函数 会比较
   * @b numVertices,
   * @b numEdges,
   * @b numPolygons,
   * @b numFaceVertices,
   * @b numUVs,
   * @b numUVSets
   * 这些属性
   * @param in_rhs 另一个obj
   * @return 是否相同
   */
  bool operator==(const maya_poly_info& in_rhs) const;
  /// @copydoc operator==(const maya_poly_info& in_rhs) const
  bool operator!=(const maya_poly_info& in_rhs) const;
  /**
   * @brief 查找maya网格信息
   * @param in_mesh_object 传入的 maya obj 用来查找属性的节点指针
   */
  void set_mesh_info(const MObject& in_mesh_object);
  /**
   * @brief 查找 maya obj 上游中使用有皮肤簇节点
   * @param in_object 传入的 maya obj 用来查找的节点指针
   * @return true 有皮肤簇
   */
  bool has_skin_cluster(const MObject& in_object);
  /**
   * @brief 查找 maya obj 下游中使用有 ql 布料节点
   * @param in_object  传入的 maya obj 用来查找的节点指针
   * @return true 有 ql 布料节点
   */
  bool has_cloth_link(const MObject& in_object);
};

}  // namespace doodle::maya_plug
