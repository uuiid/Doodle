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

  virtual void show_attribute(const episodes* in){};
  virtual void show_attribute(const project* in){};
  virtual void show_attribute(const shot* in){};
  virtual void show_attribute(const assets* in){};
  virtual void show_attribute(const season* in){};
  virtual void show_attribute(const assets_file* in){};
};

/**
 * @brief 产生编辑项目动作命令类
 * @image html attr_project.jpg 编辑项目
 * @li 添加项目 直接添加项目
 * @li 修改项目 修改改项目的一些属性(路径和名称)
 * @li 删除项目 删除项目只有在项目为空的时候才可以使用
 */
class DOODLELIB_API attr_project : public attribute_factory_interface {
  project* p_prj;
  command_ptr p_comm;

 public:
  attr_project();

  void render() override;
  void show_attribute(const project*  in) override;
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
  entt::entity p_data;
  command_ptr p_comm;
 public:
  attr_assets();

  void render() override;
  void show_attribute(const episodes*  in) override;
  void show_attribute(const project*  in) override;
  void show_attribute(const shot*  in) override;
  void show_attribute(const assets*  in) override;
  void show_attribute(const season*  in) override;
};

/**
 * @brief 产生修改资产的命令
 * @image html comm_ass_file_attr.jpg 编辑面板
 *
 * @li 资产文件的修改
 * @image html comm_ass_file_attr.jpg 编辑面板
 * @li @b 添加 \n
 *  添加一个条目,一个没有文件的资产条目
 * @li @b 更改时间
 *  更改文件时间, 这个基本用不到,但是在生成表格时还是有用的
 *
 * @li @b 删除 \n
 * 删除时不会删除文件只是在软件中不去显示条目
 * @li @b 添加注释
 * 添加一条注释在条目中
 *
 * @li 上传文件 \n
 * @note 上传文件时,需要选中文件,而不是文件夹,对于特殊的文件软件会自动检查
 *  * ue4文件 ue文件上传时, 会自动附加 Content 文件夹
 * @image html attr_assets_file.jpg 自动附加情况
 *  * 图片序列是会自动更改为父文件夹
 * @image html updata_image_sequence.jpg 自动更改为父文件夹
 *  * 在选中图片序列时，也会自动展示出上传拍屏的选项
 *
 * @li @b 上传拍屏
 * 上传拍屏时， 会自动将图片序列转换为视频
 * @image html updata_image_sequence.jpg 自动更改为父文件夹
 *  * **不上传** 不上传， 这时我们直接会在本地将视频自动合成
 *  * **不上传源文件** 只上传拍屏， 不上传图片序列
 *  * **上传视频** 将选中的条目中的文件路径替换为上传路径
 * @note 上传拍屏相当于 \n
 * 本地解析路径 \n
 *    | \n
 * 识别文名称 \n
 *    | \n
 * 确定文件所在集数和镜头 \n
 *    | \n
 * 生成水印 \n
 *    | \n
 * 本地合成拍屏 \n
 *    | \n
 * 上传文件 \n
 *    | \n
 * 上传源文件 \n
 *
 */
class DOODLELIB_API attr_assets_file : public attribute_factory_interface {
  assets_file* p_data;
  command_ptr p_comm;

 public:
  attr_assets_file();

  void render() override;
  // void show_attribute(const episodes*  in) override;
  // void show_attribute(const shot*  in) override;
  // void show_attribute(const assets*  in) override;
  // void show_attribute(const season*  in) override;
  void show_attribute(const assets_file*  in) override;
};

}  // namespace  doodle
