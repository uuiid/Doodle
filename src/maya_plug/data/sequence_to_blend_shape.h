//
// Created by TD on 2022/8/15.
//

#pragma once

#include <maya_plug/main/maya_plug_fwd.h>

namespace doodle::maya_plug {

/**
 * @brief 将一系列网格变形动画转换为混合变形节点
 */
class sequence_to_blend_shape {
 private:
  class impl;
  std::unique_ptr<impl> ptr;

  static void to_work_zero(const MDagPath& in_path);

 public:
  sequence_to_blend_shape();
  virtual ~sequence_to_blend_shape();

  sequence_to_blend_shape(sequence_to_blend_shape&& in) noexcept;
  sequence_to_blend_shape& operator=(sequence_to_blend_shape&& in) noexcept;

  /**
   * @brief 设置完成结果后附加的父路径
   * @param in_path 传入的父dag路径
   */
  void parent_attr(const MDagPath& in_path);
  /**
   * @brief 设置变形目标的原型
   * @param in_path 传入的dag路径
   */
  void select_attr(const MDagPath& in_path);

  void attach_parent();

  MDagPath& select_attr();

  /**
   * @brief 创建绑定mesh(复制出去)
   */
  void create_bind_mesh();
  /**
   * @brief 创建混合变形节点(每帧一个, );
   * @warning 需要 先使用 MAnimControl::setCurrentTime(MTime{boost::numeric_cast<std::double_t>(i), MTime::uiUnit()});
   * 设置时间
   */
  void create_blend_shape_mesh();
  /**
   * @brief 创建混合变形节点(每帧一个, );
   * @param 使用上下文的重载
   */
  void create_blend_shape_mesh(const MDGContextGuard&, std::size_t in_index);
  /**
   * @brief 最后创建混合变形
   * 调用顺序方法
   * @code
   * sequence_to_blend_shape l{};
   * MGlobal::viewFrame(bind_time_frame);
   * l.create_bind_mesh();
   * for(auto i: std::vector<MTime>{anim_ranges}){
   *  MAnimControl::setCurrentTime(i);
   *  create_blend_shape_mesh();
   * }
   * l.create_blend_shape_anim(std::int64_t in_begin_time,
   * std::int64_t in_end_time,
   * MDagModifier& in_dg_modidier);
   *
   * @endcode
   *
   */
  void create_blend_shape();

  void delete_select_node();

  /**
   * @brief 创建anim
   * @param in_begin_time 开始时间
   * @param in_end_time 结束时间
   * @param in_dg_modidier dag修改器(方便撤销)
   */
  void create_blend_shape_anim(std::int64_t in_begin_time, std::int64_t in_end_time, MDagModifier& in_dg_modidier);

  /**
   * @brief 删除创建的辅助融合变形的节点
   */
  void delete_create_blend_shape_mesh();
  /**
   * @brief 删除创建的绑定节点
   */
  void delete_bind_mesh();
};

}  // namespace doodle::maya_plug
