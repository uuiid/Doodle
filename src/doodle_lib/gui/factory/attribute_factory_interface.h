//
// Created by TD on 2021/9/23.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
namespace doodle {

class DOODLELIB_API attribute_factory_interface
    : public details::no_copy,
      public std::enable_shared_from_this<attribute_factory_interface> {
 public:
  virtual void render() = 0;

  virtual void show_attribute(const episodes_ptr& in){};
  virtual void show_attribute(const project_ptr& in){};
  virtual void show_attribute(const shot_ptr& in){};
  virtual void show_attribute(const assets_ptr& in){};
  virtual void show_attribute(const season_ptr& in){};
  virtual void show_attribute(const assets_file_ptr& in){};
};

/**
 * @brief 产生编辑项目动作命令类
 * @image html attr_project.jpg 编辑项目
 * @li 添加项目 直接添加项目
 * @li 修改项目 修改改项目的一些属性(路径和名称)
 * @li 删除项目 删除项目只有在项目为空的时候才可以使用
 */
class DOODLELIB_API attr_project : public attribute_factory_interface {
  project_ptr p_prj;
  command_ptr p_comm;

 public:
  attr_project();

  void render() override;
  void show_attribute(const project_ptr& in) override;
};
/**
 * @brief 产生编辑资产动作命令类
 * @image html attr_assets.jpg 编辑资产的一些属性或者添加资产
 *
 * @li @b 季数 \n
 *  * 添加    添加时是在选择的物体的子物体中添加的
 *  * 批量添加 批量添加和添加一样 只是会有一个结束项, **并且结束项需要大于开始项**
 *  * 删除    删除功能只有在季数下方没有任何子项是才会出现
 *  * 修改
 *    * 修改可以修改季数
 *    * 修改有需要点击修改按钮
 * @li @b 集数 \n
 *  * 添加    添加时是在选择的物体的子物体中添加的
 *  * 批量添加 批量添加和添加一样 只是会有一个结束项, **并且结束项需要大于开始项**
 *  * 删除    删除功能只有在季数下方没有任何子项是才会出现
 *  * 修改
 *    * 修改可以修改季数
 *    * 修改有需要点击修改按钮
 * @li @b 镜头 \n
 *  * 添加    添加时是在选择的物体的子物体中添加的
 *  * 批量添加 批量添加和添加一样 只是会有一个结束项, **并且结束项需要大于开始项**
 *  * 删除    删除功能只有在季数下方没有任何子项是才会出现
 *  * 修改
 *    * 修改可以修改季数
 *    * 修改有需要点击修改按钮
 * @li @b 资产 \n
 *  * 添加    添加时是在选择的物体的子物体中添加的
 *  * 批量添加 批量添加和添加一样
 *  * 删除    删除功能只有在季数下方没有任何子项是才会出现
 *  * 修改
 *    * 修改可以修改季数
 *    * 修改有需要点击修改按钮
 * @li @b 工具类-批量添加ue4镜头 \n
 * @note 这里ue路径指定为ue4项目,是必填项
 *  * 将选中的镜头批量添加到ue4中
 *  * 注意选择的资产, 会从子项中寻找所有的镜头
 * 
 */
class DOODLELIB_API attr_assets : public attribute_factory_interface {
  metadata_ptr p_data;
  command_ptr p_comm;

 public:
  attr_assets();

  void render() override;
  void show_attribute(const episodes_ptr& in) override;
  void show_attribute(const project_ptr& in) override;
  void show_attribute(const shot_ptr& in) override;
  void show_attribute(const assets_ptr& in) override;
  void show_attribute(const season_ptr& in) override;
};

class DOODLELIB_API attr_assets_file : public attribute_factory_interface {
  assets_file_ptr p_data;
  command_ptr p_comm;

 public:
  attr_assets_file();

  void render() override;
  // void show_attribute(const episodes_ptr& in) override;
  // void show_attribute(const shot_ptr& in) override;
  // void show_attribute(const assets_ptr& in) override;
  // void show_attribute(const season_ptr& in) override;
  void show_attribute(const assets_file_ptr& in) override;
};

}  // namespace  doodle
