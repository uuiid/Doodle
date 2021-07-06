//
// Created by TD on 2021/6/28.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

#include <nana/gui.hpp>
#include <nana/gui/widgets/menu.hpp>
namespace doodle {

class DOODLELIB_API menu_factory_base {
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

  virtual void create_menu(const ProjectPtr& in_ptr)    = 0;
  virtual void create_menu(const AssetsPtr& in_ptr)     = 0;
  virtual void create_menu(const EpisodesPtr& in_ptr)   = 0;
  virtual void create_menu(const ShotPtr& in_ptr)       = 0;
  virtual void create_menu(const AssetsFilePtr& in_ptr) = 0;

 public:
  explicit menu_factory_base(nana::window in_window);

  void set_metadate(const MetadataPtr& in_ptr, const MetadataPtr& in_parent = {});

  void operator()(nana::menu& in_menu);
};

class DOODLELIB_API menu_factory : public menu_factory_base {
  friend Project;
  friend Assets;
  friend Episodes;
  friend Shot;
  friend AssetsFile;

 public:
  explicit menu_factory(nana::window in_window);

  virtual void create_prj();

 protected:
  virtual void create_menu(const ProjectPtr& in_ptr) override;
  virtual void create_menu(const AssetsPtr& in_ptr) override;
  virtual void create_menu(const EpisodesPtr& in_ptr) override;
  virtual void create_menu(const ShotPtr& in_ptr) override;
  virtual void create_menu(const AssetsFilePtr& in_ptr) override;
  void create_delete_assets();
};

class DOODLELIB_API dragdrop_menu_factory : public menu_factory_base {
  std::vector<FSys::path> p_paths;

  friend Project;
  friend Assets;
  friend Episodes;
  friend Shot;
  friend AssetsFile;

  void create_menu(const ProjectPtr& in_ptr) override;
  void create_menu(const AssetsPtr& in_ptr) override;
  void create_menu(const EpisodesPtr& in_ptr) override;
  void create_menu(const ShotPtr& in_ptr) override;
  void create_menu(const AssetsFilePtr& in_ptr) override;

 public:
  explicit dragdrop_menu_factory(nana::window in_window)
      : menu_factory_base(in_window),
        p_paths(){};

  void set_drop_file(const std::vector<FSys::path>& in_path);
};
}  // namespace doodle
