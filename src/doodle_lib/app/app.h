
//
// Created by TD on 2022/1/18.
//

#pragma once
#include <doodle_lib/core/app_command_base.h>

namespace doodle {
/**
 * @addtogroup 主要程序
 *
 *
 * @section 基本概念
 * @li @b 实体 一个不可分割的最小单位， 可以表示一个对象
 * @li @b 组件 一个实体可以有多个组件，组件表示了具体的数据和概念
 * @li @b 注册表 存放所有实体和组件的池
 * @li @b 句柄 寻找实体所使用的对象
 *
 * @section 基本逻辑
 *
 * @note 在整个软件中， 使用实体表示所有的对象，组件表示表示具体的 项目， 文件，图标等,
 * 在使用时, 有几个比较重要的组件
 * @li 季数 代表了对象所属的季数
 * @li 集数 代表了对象所属的集数
 * @li 镜头 代表了对象所属的镜头
 * @li 资产(自定义分类) 这个资产类别是一个标签树类别  \n
 *  比如一个对象可以有标签 <b> 模型 -> 人物模型 -> 主角 -> 受伤版 </b> \n
 *  其中标签的关系可以具有父子级关系， 最高层是模型， 最底层是受伤版
 * @li 文件 文件代表了实际的路径， 优先生成相对于根目录的路径，
 * @li 图标 文件的简单的图片表示
 * @li 时间, 记录实体中文件组件的修改时间(依赖文件组件) \n
 *
 *
 *
 * @ref 如何查询计算时间
 * @ref 软件中的新项目是什么
 *
 *
 * @par 基本窗口
 *
 * @ref 项目窗口 窗口中显示了当前打开的项目
 *
 * @ref 过滤窗口 使用过滤器搜索整个项目中的实体
 * 在过滤窗口中, 可以叠加使用过滤器, 当叠加使用过滤器时,
 * 多个过滤器将按照从上到下的顺序进行过滤,
 * <b> 在使用 资产(自定义) 分类过滤器时, 添加的自定义资产如果没有实体使用, 将会在下次刷新时去除 </b>
 * 同时还有排序过滤器, 这种过滤器只进行排序, 并不会实际进行过滤,
 * 可以和其他过滤器混用
 *
 * @ref 实体窗口 显示过滤器观察到的实体,
 * 图片将在后台方式加载,显示的顺序并不会按照排序显示
 * 在右键中截图和打开实体
 *
 * @ref 编辑窗口 可以创建实体 和 对实体进行编辑,
 * 创建实体时, 有可以创建空实体和根据文件创建实体这两种方式
 * 编辑实体时, 可以多选编辑, 但是将会把实体的组件全部编辑为统一值
 *
 * @ref 设置窗口 软件的各种设置在这个窗口中进行
 *
 * @ref 项目设置窗口 关于项目的设置将在这个窗口中进行, 其中每个项目的设置各不相同,可以分别设置
 *
 * @section 基本操作
 * @subsection 打开项目
 * 打开的项目可以在最近的项目中显示 或者之间双击储存库文件
 * @image html doodle_image/open_prj.gif 打开项目 width=90%
 * @image html doodle_image/open_prj2.gif 打开项目 width=90%
 *
 * @subsection 搜索实体
 * 搜索实体时, 可以多个过滤器进行叠加
 * @image html doodle_image/filter_win.gif 搜索 width=90%
 *
 * @subsection 选择实体
 * 选择实体中 按shift 进行多选 按ctrl反选
 * @image html doodle_image/select_entt.gif 选择 width=90%
 *
 * @subsection 添加实体
 * @note 在添加实体时, 最好打开扫描文件时间的选项, 计算时间修正提交时间
 *
 * @image html doodle_image/add_entt.gif 选择 width=90%
 *
 * @subsection 编辑组件
 * @warning 编辑后需求使用 ctrl + s 键保存
 * @note
 * @li 在设置中, 需要填入自己所属的部门, 而不是选择,
 * @li 在编辑中, 可以修正和调整提交的时间,  同时可以记录备注和等级等等
 *
 * @image html doodle_image/edit_entt.gif 选择 width=90%
 *
 * @subsection 导出为csv
 * @warning 导出时实体必须有文件路径组件, 所有不具有文件路径组件的实体都将被忽略
 * @note
 * @li 在导出为csv 中, 可以使用顶级的分类作为 项目名称, 然后在导出时去除分类的最上层
 *
 *
 * @image html doodle_image/export_csv.gif 选择 width=90%
 *
 *
 * @section 插件系统
 * @li @b maya插件 将自身集成到maya中,同时会显示一些maya特有的功能
 * @li @b ue4插件 将自身的一部分集成到ue4中, 提供一些ue4特有的功能
 *
 * @section 小工具
 * @li 安装maya插件
 * @li 安装ue4 插件
 * @li 更新ue4缓存写入位置(将缓存从C:/盘更新到项目目录)
 * @li 删除ue4C盘缓存
 */

/**
 * @brief
 * # 主工具
 * * 辅助安装maya插件
 * * 辅助安装ue4插件
 * * 清除ue4缓存
 * * 修改ue4缓存位置
 * * 批量导出fbx
 * * 批量进行布料解算
 * * 创建ue4关卡
 * * 导入ue4 fbx和abc文件
 * * 创建拍屏并合成水印
 * * 连接拍屏
 * * 模型库工具（正在开发）
 *
 * # ue工具
 * * 批量修正材质名称和材质属性
 * * 导入fbx和abc
 * * 传递材质
 * *
 *
 *
 *
 */
class DOODLELIB_API app : public app_command_base {
 protected:
  win::wnd_class p_win_class;

  std::shared_ptr<win::d3d_device> d3d_deve;
  bool p_show_err;

 private:
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  explicit app(const win::wnd_instance& in_instance = nullptr,
               const win::wnd_handle& in_parent     = nullptr);

  ~app() override;
  win::wnd_handle p_hwnd;
  ::ID3D11Device* d3dDevice;
  ::ID3D11DeviceContext* d3dDeviceContext;

  static app& Get();
  bool valid() const override;

  void set_title(const std::string& in_title);

  virtual void close_windows();
  virtual void show_windows();
  virtual bool set_parent(win::wnd_handle in_parent);

 protected:
  virtual void load_windows();
  void load_back_end() override;
  virtual bool chick_authorization() override;
  virtual void post_constructor() override;
};
}  // namespace doodle
