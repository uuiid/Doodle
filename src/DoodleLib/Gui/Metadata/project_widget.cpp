//
// Created by TD on 2021/6/28.
//

#include "project_widget.h"

#include <Gui/action/action_import.h>
#include <Gui/factory/menu_factory.h>
#include <Metadata/AssetsFile.h>
#include <Metadata/Comment.h>
#include <Metadata/Project.h>
#include <Metadata/TimeDuration.h>
#include <libWarp/nana_warp.h>
namespace doodle {

nana::listbox::oresolver& operator<<(nana::listbox::oresolver& oor, const ProjectPtr& prj) {
  oor << prj->showStr();
  oor << prj->getPath().generic_string();
  oor << prj->str();
  return oor;
}
nana::listbox::iresolver& operator>>(nana::listbox::iresolver& oor, ProjectPtr& prj) {
  return oor;
}
nana::listbox::oresolver& operator<<(nana::listbox::oresolver& oor, const AssetsFilePtr& in_file) {
  oor << in_file->getId()
      << in_file->getVersionStr();
  auto k_com = in_file->getComment();
  if (k_com.empty())
    oor << "none";
  else
    oor << k_com.back()->getComment();

  oor << in_file->getTime()->showStr()
      << in_file->getUser()
      << (!in_file->getPathFile().empty());

  return oor;
}
nana::listbox::iresolver& operator>>(nana::listbox::iresolver& oor, AssetsFilePtr& in_file) {
  return oor;
}

project_widget::project_widget(nana::window in_window)
    : details::pej_widget_base(),
      p_list_box(in_window) {
  p_list_box.append_header("名称");
  p_list_box.append_header("根目录");
  p_list_box.append_header("英文名称");

  p_list_box.clear();

  details::draw_guard<nana::listbox> k_guard{p_list_box};

  auto& k_container = CoreSet::getSet().p_project_vector;
  for (auto& k_i : k_container) {
    auto k_item    = p_list_box.at(0).append(k_i, true).pos();
    k_i->user_date = k_item;
    std::weak_ptr<Project> k_ptr{k_i};
    k_i->sig_change.connect([this, k_ptr]() {
      p_list_box.at(std::any_cast<nana::listbox::index_pair>(k_ptr.lock()->user_date))
          .text(0, k_ptr.lock()->showStr())
          .text(1, k_ptr.lock()->getPath().generic_string());
    });
  }
  p_list_box.events().selected([this](const nana::arg_listbox& in_) {
    auto k_prj = in_.item.value<ProjectPtr>();
    if (!k_prj) {
      DOODLE_LOG_WARN("选中项目为空")
      return;
    }
    sig_selected(k_prj, in_.item.selected());
  });

  ///连接项目事件
  k_container.sig_swap.connect(
      [this](const std::vector<ProjectPtr>& val) {
        details::draw_guard k_draw_guard{p_list_box};
        p_list_box.clear();
        for (const auto& k_item : val)
          p_list_box.at(0).append(k_item, true);
      });

  k_container.sig_push_back.connect(
      [this](const ProjectPtr& val) {
        p_list_box.at(0).append(val, true);
      });

  k_container.sig_erase.connect(
      [this](const ProjectPtr& val) {
        for (auto& k_i : p_list_box.at(0)) {
          const auto k_ptr = k_i.value<ProjectPtr>();
          if (k_ptr == val) {
            p_list_box.erase(k_i);
            break;
          }
        }
      });

  //  p_list_box.events()
  //      .selected([](const nana::arg_listbox& in_) {
  //        in_.item.value<ProjectPtr>();
  //      });

  p_list_box.events().mouse_down(menu_assist{[this](const nana::arg_mouse& in_) {
    p_menu.clear();

    auto k_factory  = std::make_shared<menu_factory_project>(in_.window_handle);
    this->p_factory = k_factory;

    auto k_selected = p_list_box.selected();
    // MetadataPtr k_ptr{};
    if (k_selected.empty())
      k_factory->create_prj();
    else {
      auto k_pair = k_selected.at(0);
      auto k_ptr  = p_list_box.at(k_pair).value<ProjectPtr>();
      k_factory->set_metadate(k_ptr);
      k_ptr->create_menu(k_factory);
    }

    (*k_factory)(p_menu);
    p_menu.popup(in_.window_handle, in_.pos.x, in_.pos.y);
  }});

  //  nana::API::refresh_window(p_list_box);
}
nana::listbox& project_widget::get_widget() {
  return p_list_box;
}

assets_widget::assets_widget(nana::window in_window)
    : details::pej_widget_base(),
      p_tree_box(in_window),
      p_root(),
      p_conn() {
  p_tree_box.events().selected([this](const nana::arg_treebox& in_) {
    auto k_ptr = in_.item.value<MetadataPtr>();
    if (k_ptr)
      sig_selected(k_ptr, in_.item.selected());
    DOODLE_LOG_INFO("选中 {}", in_.item.key())
  });
  /**
   * 连接菜单事件
   */
  p_tree_box.events().mouse_down(menu_assist{[this](const nana::arg_mouse& in_) {
    p_menu.clear();

    auto k_factory  = std::make_shared<menu_factory_assets>(in_.window_handle);
    this->p_factory = k_factory;
    auto k_selected = p_tree_box.selected();
    MetadataPtr k_ptr{};
    /// 选择为空, 获取根, 如果都是空, 直接返回
    if (k_selected.empty()) {
      k_ptr = p_root;
      k_factory->set_metadate({}, p_root);
    } else {
      k_ptr = k_selected.value<MetadataPtr>();
      /// 有选择的情况下， 使用选择作为父级
      k_factory->set_metadate(k_ptr, k_ptr);
    }

    if (!k_ptr)
      return;
    k_ptr->create_menu(k_factory);

    (*k_factory)(p_menu);
    p_menu.popup(in_.window_handle, in_.pos.x, in_.pos.y);
  }});

  /**
   * 连接点击事件
   */
  p_tree_box.events()
      .expanded([this](const nana::arg_treebox& in_) {
        if (!in_.operated)  //如果不是展开节点就直接返回
          return;
        auto k_proxy = in_.item;
        DOODLE_LOG_INFO("扩展 {}", k_proxy.key())

        k_proxy.clear();

        auto k_me = k_proxy.value<MetadataPtr>();

        add_nodes(k_me);
      });

  //  p_tree_box.events().selected([this](const nana::arg_treebox& in_) {
  //    auto k_string = p_tree_box.make_key_path(in_.item, "/");
  //    DOODLE_LOG_INFO("key path: {}", k_string);
  //    DOODLE_LOG_INFO("key: {}", p_tree_box.find(k_string).key())
  //  });
}
void assets_widget::set_ass(const MetadataPtr& in_project_ptr) {
  p_conn.clear();

  if (!in_project_ptr) {
    DOODLE_LOG_INFO("传入空指针")
    return;
  }
  p_root = in_project_ptr;

  if (!in_project_ptr->hasChild()) {
    p_tree_box.clear();
    return;
  }

  p_tree_box.clear();

  add_nodes(in_project_ptr);
}

void assets_widget::clear() {
  p_conn.clear();
  p_root.reset();
  p_tree_box.clear();
}

nana::treebox& assets_widget::get_widget() {
  return p_tree_box;
}
void assets_widget::install_solt(const MetadataPtr& in_ptr) {
  std::weak_ptr<Metadata> k_ptr{in_ptr};

  p_conn.emplace_back(
      boost::signals2::scoped_connection{
          in_ptr->child_item.sig_sort.connect([this, k_ptr](const std::vector<MetadataPtr>& in_) {
            add_nodes(k_ptr.lock());
          })});

  p_conn.emplace_back(boost::signals2::scoped_connection{

      in_ptr->child_item.sig_push_back.connect([this](const MetadataPtr& val) {
        auto k_p = val->getParent();
        nana::treebox::item_proxy k_item{};
        if (k_p->hasParent()) {
          auto k_str = std::any_cast<std::string>(k_p->user_date);
          k_item     = p_tree_box.find(k_str);
        }
        add_node(val, k_item);
      })});
  p_conn.emplace_back(
      boost::signals2::scoped_connection{
          in_ptr->child_item.sig_erase.connect([this](const MetadataPtr& val) {
            if (!val->user_date.has_value())
              return;
            if (details::is_class<AssetsFile>(val))
              return;

            p_tree_box.erase(std::any_cast<std::string>(val->user_date));
          })

      });

  p_conn.emplace_back(
      boost::signals2::scoped_connection{
          in_ptr->child_item.sig_swap.connect([this, k_ptr](const std::vector<MetadataPtr>& val) {
            add_nodes(k_ptr.lock());
          })

      });
  p_conn.emplace_back(
      boost::signals2::scoped_connection{
          in_ptr->sig_change.connect([this, k_ptr]() {
            try {
              if (details::is_class<Project>(k_ptr.lock()) || details::is_class<AssetsFile>(k_ptr.lock()))
                return;
              auto str = std::any_cast<std::string>(k_ptr.lock()->user_date);
              if (str.empty())
                return;

              p_tree_box.find(str).text(k_ptr.lock()->showStr());
              nana::API::update_window(p_tree_box);
            } catch (std::bad_any_cast& error) {
              DOODLE_LOG_WARN("不是可以转换的 {} {}", error.what(), k_ptr.lock()->showStr());
            }
          })});
}

void assets_widget::add_nodes(const MetadataPtr& in_parent) {
  details::draw_guard<nana::treebox> k_guard{p_tree_box};
  nana::treebox::item_proxy k_proxy{};
  in_parent->select_indb();

  in_parent->sortChildItems();
  if (in_parent->hasParent()) {  // 有父物体的情况一定不是根
    k_proxy = p_tree_box.find(std::any_cast<std::string>(in_parent->user_date));
  } else {  //这个传入的时根物体节点（项目节点）
    p_tree_box.clear();
    install_solt(in_parent);
  }
  for (auto& k_i : in_parent->child_item) {
    add_node(k_i, k_proxy);
  }
}

void assets_widget::add_node(const MetadataPtr& in_node, nana::treebox::item_proxy& in_parent) {
  if (details::is_class<AssetsFile>(in_node))
    return;

  nana::treebox::item_proxy k_item{};
  if (in_parent.empty()) {
    k_item = p_tree_box.insert(in_node->getIdStr(), in_node->showStr());
    k_item.value(in_node);
  } else {
    k_item = in_parent.append(in_node->getIdStr(), in_node->showStr(), in_node);
  }
  in_node->user_date = p_tree_box.make_key_path(k_item, "/");
  install_solt(in_node);

  DOODLE_LOG_INFO("树文件路径: {}", p_tree_box.make_key_path(k_item, "/"));
  if (in_node->hasChild()) {
    k_item.append("none", "none");
  }
}

assets_attr_widget::assets_attr_widget(nana::window in_window)
    : details::pej_widget_base(),
      p_list_box(in_window),
      p_assets(),
      p_root(),
      p_conn() {
  p_list_box.append_header("id");
  p_list_box.append_header("版本");
  p_list_box.append_header("评论", 300);
  p_list_box.append_header("时间", 130);
  p_list_box.append_header("制作人", 70);
  p_list_box.append_header("有文件");

  p_list_box.enable_dropfiles(true);

  p_list_box.events().mouse_dropfiles.connect([this](const nana::arg_dropfiles& in_) {
    p_menu.clear();
    auto k_factory = std::make_shared<dragdrop_menu_factory>(in_.window_handle);

    this->p_factory = k_factory;

    k_factory->set_drop_file(in_.files);
    k_factory->set_metadate({}, p_root);
    auto k_selected = p_list_box.selected();
    MetadataPtr k_ptr{};

    /// 如果有选择就获得选择, 没有就获得根, 再没有就返回
    if (!k_selected.empty()) {
      auto k_pair = k_selected.at(0);
      k_ptr       = p_list_box.at(k_pair).value<AssetsFilePtr>();
      k_factory->set_metadate(k_ptr, p_root);
    } else {
      k_ptr = p_root;
      k_factory->set_metadate({}, p_root);
    }

    if (!p_root) {
      DOODLE_LOG_WARN("没有文件获得选择或者指针， 直接返回")
      return;
    }
    k_ptr->create_menu(k_factory);

    (*k_factory)(p_menu);
    if (p_menu.size() == 0) {
      DOODLE_LOG_WARN("没有创建出菜单， 直接返回")
      return;
    }
    p_menu.popup(in_.window_handle, in_.pos.x, in_.pos.y);
  });

  p_list_box.events().mouse_down.connect(menu_assist{[this](const nana::arg_mouse& in_) {
    p_menu.clear();
    auto k_factory  = std::make_shared<menu_factory_assets_attr>(in_.window_handle);
    this->p_factory = k_factory;
    auto k_selected = p_list_box.selected();
    MetadataPtr k_ptr{};

    /// 如果有选择就获得选择, 没有就获得根, 再没有就返回
    if (!k_selected.empty()) {
      auto k_pair = k_selected.at(0);
      k_ptr       = p_list_box.at(k_pair).value<AssetsFilePtr>();
      k_factory->set_metadate(k_ptr, p_root);
    } else {
      k_ptr = p_root;
      k_factory->set_metadate({}, p_root);
    }
    if (!k_ptr) {
      DOODLE_LOG_WARN("没有文件获得选择或者指针， 直接返回")
      return;
    }
    k_ptr->create_menu(k_factory);
    (*k_factory)(p_menu);
    p_menu.popup(in_.window_handle, in_.pos.x, in_.pos.y);
  }});
}
void assets_attr_widget::set_ass(const MetadataPtr& in_ptr) {
  clear();

  if (!in_ptr) {
    DOODLE_LOG_WARN("传入空镜头")
    return;
  }
  p_root = in_ptr;
  p_root->select_indb();
  p_root->sortChildItems();

  if (!p_root->hasChild()) {
    DOODLE_LOG_WARN("选中项没有子项")
    return;
  }

  details::draw_guard<nana::listbox> k_guard{p_list_box};

  for (auto& k_i : p_root->child_item) {
    if (!details::is_class<AssetsFile>(k_i)) {
      DOODLE_LOG_WARN("项目不是文件项")
      continue;
    }
    auto k_file = std::dynamic_pointer_cast<AssetsFile>(k_i);
    if (!k_file) {
      continue;
    }
    // 在这里我们插入文件
    p_list_box.assoc(k_file->getDepartment())
        .text(std::string{magic_enum::enum_name(k_file->getDepartment())})
        .append(k_file, true)
        .bgcolor(k_file->getPathFile().empty() ? nana::colors::light_coral : nana::colors::light_green);

    install_sig_one(k_file);
  }
  install_sig();
}
void assets_attr_widget::install_sig() {
  p_conn.emplace_back(boost::signals2::scoped_connection{
      p_root->child_item.sig_push_back.connect([this](const MetadataPtr& in_) {
        if (!details::is_class<AssetsFile>(in_))
          return;
        auto k_file = std::dynamic_pointer_cast<AssetsFile>(in_);
        // 在这里我们插入文件
        p_list_box.assoc(k_file->getDepartment())
            .text(std::string{magic_enum::enum_name(k_file->getDepartment())})
            .append(k_file, true)
            .bgcolor(k_file->getPathFile().empty() ? nana::colors::light_coral : nana::colors::light_green);

        install_sig_one(k_file);
      })});
  p_conn.emplace_back(boost::signals2::scoped_connection{
      p_root->child_item.sig_insert.connect([this](const MetadataPtr& in_) {
        if (!details::is_class<AssetsFile>(in_))
          return;
        auto k_file = std::dynamic_pointer_cast<AssetsFile>(in_);
        // 在这里我们插入文件
        p_list_box.assoc(k_file->getDepartment())
            .text(std::string{magic_enum::enum_name(k_file->getDepartment())})
            .append(k_file, true)
            .bgcolor(k_file->getPathFile().empty() ? nana::colors::light_coral : nana::colors::light_green);

        install_sig_one(k_file);
      })});
  p_conn.emplace_back(boost::signals2::scoped_connection{
      p_root->child_item.sig_erase.connect([this](const MetadataPtr& in_) {
        if (!details::is_class<AssetsFile>(in_))
          return;
        auto k_file = std::dynamic_pointer_cast<AssetsFile>(in_);
        for (auto& k_i : p_list_box.assoc(k_file->getDepartment())) {
          if (k_i.value<AssetsFilePtr>() == k_file) {
            p_list_box.erase(k_i);
          }
        }
      })});
}
void assets_attr_widget::install_sig_one(std::shared_ptr<AssetsFile>& k_file) {
  std::weak_ptr<AssetsFile> k_ptr{k_file};
  p_conn.emplace_back(boost::signals2::scoped_connection{
      k_file->sig_change.connect([k_ptr, this]() {
        auto k_f = k_ptr.lock();
        for (auto& k_i : p_list_box.assoc(k_f->getDepartment())) {
          if (k_i.value<AssetsFilePtr>() == k_f)
            k_i.text(0, k_f->getIdStr())
                .text(1, k_f->getVersionStr())
                .text(2, k_f->getComment().empty() ? "none" : k_f->getComment().back()->getComment())
                .text(3, k_f->getTime()->showStr())
                .text(5, !k_f->getPathFile().empty() ? "true" : "false")
                .bgcolor(k_f->getPathFile().empty() ? nana::colors::light_coral : nana::colors::light_green);
        }
      })});
}

void assets_attr_widget::clear() {
  p_conn.clear();
  p_list_box.clear();
  p_list_box.erase();
  p_menu.clear();
  p_root.reset();
}
nana::listbox& assets_attr_widget::get_widget() {
  return p_list_box;
}
}  // namespace doodle
