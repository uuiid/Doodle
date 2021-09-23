//
// Created by TD on 2021/9/23.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
namespace doodle {

class attribute_factory_interface
    : public details::no_copy,
      public std::enable_shared_from_this<attribute_factory_interface> {
 public:
  virtual void render() = 0;

  virtual void show_attribute(const EpisodesPtr& in){};
  virtual void show_attribute(const ProjectPtr& in){};
  virtual void show_attribute(const ShotPtr& in){};
  virtual void show_attribute(const AssetsPtr& in){};
  virtual void show_attribute(const AssetsFilePtr& in){};
  virtual void show_attribute(const season_ptr& in){};
};

class attr_project : public attribute_factory_interface {
  ProjectPtr p_prj;
  command_meta_ptr p_comm;
 public:
  attr_project();

  virtual void render() override;
  virtual void show_attribute(const ProjectPtr& in) override;
};

}  // namespace  doodle
