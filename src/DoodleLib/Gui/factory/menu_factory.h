//
// Created by TD on 2021/6/28.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

#include <nana/gui.hpp>
#include <nana/gui/widgets/menu.hpp>
namespace doodle {
/**
 * @brief 创建各种菜单的辅助类
 *
 */
class DOODLELIB_API menu_factory_base
    : public std::enable_shared_from_this<menu_factory_base>,
      public details::no_copy {
 protected:
  CoreSet& p_set;
  std::vector<action_ptr> p_action;
  nana::window p_window;
  MetadataPtr p_metadata;
  MetadataPtr p_parent;

  friend Project;
  friend Assets;
  friend Episodes;
  friend Shot;
  friend AssetsFile;



 public:
  /**
   * @brief 这个时候使用 metadata 转发的方法
   *
   * @param in_ptr
   */

  explicit menu_factory_base(nana::window in_window);
  /**
   * @brief 设置创建各种内容所需要的数据
   * @warning 这里要注意先添加数据， 然后再开始创建菜单
   * @param in_ptr 选中的内容
   * @param in_parent 添加数据的父指针（只有在创建项目时这个可以是空）
   */
  void set_metadate(const MetadataPtr& in_ptr, const MetadataPtr& in_parent = {});
  /**
   * @brief 将创建的动作转换为菜单的便捷函数
   *
   * @param in_menu
   */
  virtual void operator()(nana::menu& in_menu);
};
/**
 * @brief 创建右键菜单
 *
 */
class DOODLELIB_API menu_factory : public menu_factory_base {
 public:
  explicit menu_factory(nana::window in_window);

  virtual void create_prj();

  virtual void create_menu(const ProjectPtr& in_ptr)  = 0;
  virtual void create_menu(const AssetsPtr& in_ptr)  = 0;
  virtual void create_menu(const EpisodesPtr& in_ptr)  = 0;
  virtual void create_menu(const ShotPtr& in_ptr)  = 0;
  virtual void create_menu(const AssetsFilePtr& in_ptr)  = 0;
  virtual void create_menu(const season_ptr& in_ptr)  = 0;

 protected:
  void create_assets();
  void create_episodes();
  void create_shot();
  void create_assets_file();
  void create_season();
  void create_assets_file_up_data();
  void create_assets_file_video_up();
  void create_assets_file_export_maya_up();
  void create_assets_file_batch_video_up();
  void create_assets_file_batch_export_maya_up();
  void create_ue4_Sequencer();

  void export_excel();

  void show_assets_file_attr();

  void modify_project_rename();
  void modify_project_set_path();
  void modify_assets_set_name();
  void modify_episode();
  void modify_season();
  void modify_shot_int();
  void modify_shot_ab();
  void modify_attr_add_com();
  void modify_attr_set_time();
  void modify_assets_file_up_data();
  void down_file();

  void delete_project();
  void delete_assets();
  void delete_assets_attr();
};

/**
 * @brief 这个是创建 项目右键菜单的方法
 *
 * @image html doodle_main_menu_factory_project.jpg width=30%
 *
 * @li 创建项目 直接创建项目， 名称使用中文， 拼音名称会自动生成，
 * @warning 请不要使用符号， 各种书名号或者其他符号等
 * @li 导出表格 导出csv格式的表格， 使用各种软件都可以打开
 * @li 重命名项目 重命名项目时 不会同时改变拼音名称
 * @li 设置路径 设置项目根路径
 *
 */
class DOODLELIB_API menu_factory_project : public menu_factory {
 public:
  explicit menu_factory_project(nana::window in_window);

  void create_menu(const ProjectPtr& in_ptr) override;
  void create_menu(const AssetsPtr& in_ptr) override;
  void create_menu(const EpisodesPtr& in_ptr) override;
  void create_menu(const ShotPtr& in_ptr) override;
  void create_menu(const AssetsFilePtr& in_ptr) override;
  void create_menu(const season_ptr& in_ptr) override;
};

/**
 * @brief 创建资产小部件右键菜单
 *
 * @image html doodle_main_menu_factory_assets.jpg  width=30%
 * @li 可以创建 @b 集数，镜头，季数 这三个是不可以是负数，
 * @li 其中镜头是可以有ab镜头的， 但是请注意不要太多， 只可以有26个
 * @li 同时是可以创建重复的镜头的， 但是如果没有特殊情况，请不要创建重复的镜头
 * @li @b 资产 是可以使用任意的名称的， 可以使用中文， 在内部会自动转换为拼音，
 * @warning 不要使用特殊字符和 @b -,*,$,%,#,还有各种中文标点符号 \n
 * 请小心多音字, 如果有多音字, 请加入 @b 后缀区分 \n
 * 集数显示为 ep0002,还有镜头显示为sc0002, 季数显示为seas_2这种, \n
 * @b 请不要将资产命名为这种类型的名称!!!!! 以便于区分
 * @li @b 删除 只有在这个项目没有子物体和子条目时才会显示
 *
 */
class DOODLELIB_API menu_factory_assets : public menu_factory {
 public:
  explicit menu_factory_assets(nana::window in_window);

  virtual void create_menu(const AssetsFilePtr& in_ptr) override;
  virtual void create_menu(const ProjectPtr& in_ptr) override;
  virtual void create_menu(const AssetsPtr& in_ptr) override;
  virtual void create_menu(const EpisodesPtr& in_ptr) override;
  virtual void create_menu(const ShotPtr& in_ptr) override;
  virtual void create_menu(const season_ptr& in_ptr) override;
};

/**
 * @brief 资产的详情信息右键菜单栏
 *
 * @image html doodle_main_menu_factory_assets_attr.jpg width=30%
 *
 * @li @b 创建资产文件 是空的， 如果想要没有文件的话直接点这个就行
 * @li @b 创建并上传文件 是上传并创建一个条目
 * @li @b 制作拍屏并上传 可以上传一个拍屏文件， 作为示意
 * @li @b 显示详细信息 可以显这个条目的详细信息
 * @li @b 添加评论 附加一条描述性的条目
 * @li @b 修改日期 将条目的日期修改到指定时间， 这个请小心使用， 在导出表格时会使用这个日期
 * @li @b 下载文件 如果在这个条目中有文件的话可以直接下载下来
 * @li @b 删除 删除这个条目， 不会删除文件
 */

class DOODLELIB_API menu_factory_assets_attr : public menu_factory {
 public:
  explicit menu_factory_assets_attr(nana::window in_window);
  virtual void create_menu(const ProjectPtr& in_ptr) override;
  virtual void create_menu(const AssetsPtr& in_ptr) override;
  virtual void create_menu(const EpisodesPtr& in_ptr) override;
  virtual void create_menu(const ShotPtr& in_ptr) override;
  virtual void create_menu(const AssetsFilePtr& in_ptr) override;
  virtual void create_menu(const season_ptr& in_ptr) override;
};
/**
 * @brief 拖拽文件时所产生的菜单
 *
 */
class DOODLELIB_API dragdrop_menu_factory : public menu_factory_base {
  std::vector<FSys::path> p_paths;

 public:

  explicit dragdrop_menu_factory(nana::window in_window)
      : menu_factory_base(in_window),
        p_paths(){};



  /**
   * @brief 传入的拖拽文件的列表
   *
   * @param in_path
   */
  void set_drop_file(const std::vector<FSys::path>& in_path);
  void drop_menu();
  void create_image_and_up();
};
}  // namespace doodle
