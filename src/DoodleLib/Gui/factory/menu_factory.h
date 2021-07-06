//
// Created by TD on 2021/6/28.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

#include <nana/gui.hpp>
#include <nana/gui/widgets/menu.hpp>
namespace doodle {

class DOODLELIB_API menu_factory {
 protected:
  CoreSet& p_set;
  std::vector<action_ptr> p_action;
  nana::window p_window;
  MetadataPtr p_metadata;

  friend Project;
  friend Assets;
  friend Episodes;
  friend Shot;
  friend AssetsFile;

  virtual void create_menu(const ProjectPtr& in_ptr){};
  virtual void create_menu(const AssetsPtr& in_ptr){};
  virtual void create_menu(const EpisodesPtr& in_ptr){};
  virtual void create_menu(const ShotPtr& in_ptr){};
  virtual void create_menu(const AssetsFilePtr& in_ptr){};

 public:
  explicit menu_factory(nana::window in_window);

  virtual void create_prj(){};

  virtual void prj_action(MetadataPtr& in_metadata, bool is_parent = false);
  virtual void file_action(MetadataPtr& in_metadata, bool is_parent = false);
  virtual void ass_action(MetadataPtr& in_metadata, bool is_parent = false);

  void operator()(nana::menu& in_menu);
};

class DOODLELIB_API dragdrop_menu_factory : public menu_factory {
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
      : menu_factory(in_window),
        p_paths(){};

  void set_drop_file(const std::vector<FSys::path>& in_path);
};
}  // namespace doodle
