//
// Created by TD on 2021/6/28.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

#include <boost/signals2.hpp>
#include <nana/gui/widgets/listbox.hpp>
#include <nana/gui/widgets/menu.hpp>
#include <nana/gui/widgets/treebox.hpp>
namespace doodle {

namespace details {
template <class widget>
class draw_guard : public no_copy {
  widget& _w;

 public:
  explicit draw_guard(widget& in_widget) : _w(in_widget) {
    _w.auto_draw(false);
  }
  ~draw_guard() {
    _w.auto_draw(true);
  }
};
}  // namespace details

nana::listbox::oresolver& operator<<(nana::listbox::oresolver& oor, const ProjectPtr& in_prj);
nana::listbox::iresolver& operator>>(nana::listbox::iresolver& oor, ProjectPtr& in_prj);

nana::listbox::oresolver& operator<<(nana::listbox::oresolver& oor, const AssetsFilePtr& in_file);
nana::listbox::iresolver& operator>>(nana::listbox::iresolver& oor, AssetsFilePtr& in_file);

//class widget {
// protected:
//  nana::window _win;
//
// public:
//  explicit widget(nana::window in_window) : _win(in_window){};
//  virtual ~widget() = default;
//};

namespace details {
/**
 * @brief 基本的主视口小部件
 * 
 */
class pej_widget_base {
 protected:
  /**
   * @brief 需要弹出的右键菜单
   * 
   */
  nana::menu p_menu;
  /**
   * @brief 右键菜单生成工厂
   * 
   */
  std::shared_ptr<menu_factory_base> p_factory;

 public:
  /**
   * @brief 点击时发出的信号 
   * @param in_ 点击时的元数据
   * @param is_selected 是点击还是取消点击 
   * 
   */
  boost::signals2::signal<void(const MetadataPtr& in_, bool is_selected)> sig_selected;

  /**
   * @brief 获得被包裹的小部件
   * 虽然时可变引用, 但是请不要随意更改值
   * 
   * @return nana::widget& 被包裹的小部件的可变引用
   */
  virtual nana::widget& get_widget() = 0;
};
}  // namespace details

/**
 * @brief 项目窗口
 * 
 * @image html doodle_project_windows.jpg  width=60%
 * 
 * @li 在项目窗口中名称可以填写为中文， 英文名称是中文的拼音， 
 * @note 如果是同音字那么他们的英文名称相同
 *  这里要注意， 如果有两项目的子全部是同音字那么一定要添加一个后缀作为区分
 * 
 * @li 除此以外项目的根目录不要随便更改， 是指向服务器中的目录
 * @li 项目的名称不要随便更改
 * @todo 英文名称和中文名称可以单独更改 
 * 
 */
class DOODLELIB_API project_widget : public details::pej_widget_base {
  nana::listbox p_list_box;

 public:
  explicit project_widget(nana::window in_window);

  nana::listbox& get_widget() override;
};

/**
 * @brief 资产窗口
 * 
 * @image html doodle_main_assets_widget.jpg  height=60%
 * 这个窗口中可以记录 @b 集数，镜头和资产名称 这几个类别
 * @note 这里资产名称和集数，镜头是上下级关系， 就像是文件夹一样，
 * 但是他们的嵌套关系可以是任意的， 比如集数下是镜头， 或者镜头下是集数， 或者资产下是镜头
 * 
 * @todo 添加季数这个类别
 * 
 * 
 */
class DOODLELIB_API assets_widget : public details::pej_widget_base {
  nana::treebox p_tree_box;

  MetadataPtr p_root;

  std::vector<boost::signals2::scoped_connection> p_conn;
  void install_solt(const MetadataPtr& in_project_ptr);
  //  nana::treebox::item_proxy append_tree(nana::treebox::item_proxy& in_proxy, MetadataPtr& in_ptr);
  void add_nodes(const MetadataPtr& in_parent);
  void add_node(const MetadataPtr& in_node, nana::treebox::item_proxy& in_parent);

 public:
  explicit assets_widget(nana::window in_window);

  void set_ass(const MetadataPtr& in_project_ptr);
  void clear();

  nana::treebox& get_widget() override;
};

class DOODLELIB_API assets_attr_widget : public details::pej_widget_base {
  nana::listbox p_list_box;
  MetadataPtr p_root;

  std::vector<AssetsFilePtr> p_assets;
  std::vector<boost::signals2::scoped_connection> p_conn;
  void install_sig();
  void install_sig_one(std::shared_ptr<AssetsFile>& k_file);

 public:
  explicit assets_attr_widget(nana::window in_window);
  void set_ass(const MetadataPtr& in_ptr);
  void clear();
  nana::listbox& get_widget() override;
};
}  // namespace doodle
