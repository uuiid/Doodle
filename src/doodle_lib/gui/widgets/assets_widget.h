//
// Created by TD on 2021/9/16.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/base_windwos.h>

#include <boost/signals2.hpp>
namespace doodle {
/**
 * @brief 资产显示树
 * @image html attr_project.jpg 资产树 
 * @li 这里只显示资产树, 可以类比为文件夹树
 * 
 */
class DOODLELIB_API assets_widget : public metadata_widget {
  registry_ptr reg;
  entt::entity p_root;
  /**
   * @brief  我们在这里使用上一帧的选中物体开始测试是否选中而不是使用这一帧的
   * 
   */
  std::set<entt::entity> p_all_old_selected;
  /**
   * @brief 递归的加载树节点
   * 
   * @param in_ptr 传入父物体
   */
  void load_meta(const entt::entity& in_ptr);
  /**
   * @brief 判断是否是选中对象,
   * 
   * @param in_ptr 是否是选中的
   * @return true 选中了
   * @return false 没有选中
   */
  bool is_select(const entt::entity& in_ptr);

  /**
   * @brief 多重选择时使用, 多重选择是不发出任何信号的
   * 
   * @param in_ptr 传入判断小部件成功时需要的数据，
   *  主要时加入 p_all_selected 属性和 p_meta属性
   */
  void check_item(const entt::entity& in_ptr);
  /**
   * @brief 在去除选择和选择时使用, 去除选择时, 不发出选择更改信号
   * 
   * @param in_ptr 检查点击, 这个要和检查项目分开判断
   */
  void check_item_clicked(const entt::entity& in_ptr);

 public:
  assets_widget();
  void frame_render() override;
  /**
   * @brief 设置这个小部件要创建的树的根属性
   * 
   * @param in_ptr 
   */
  void set_metadata(const entt::entity& in_ptr);

  /**
   * @brief 设置选择, 这里是设置单选的方式
   * 
   * @param in_ptr 
   */
  void set_select(const entt::entity& in_ptr);

  /**
   * @brief 多选的物体
   * 
   */
  std::set<entt::entity> p_all_selected;
  /**
   * @brief 单选的物体
   * 
   */
  entt::entity p_meta;
  /**
   * @brief 单选时切换的信号
   * 
   */
  boost::signals2::signal<void(const entt::entity&)> select_change;
};
}  // namespace doodle
