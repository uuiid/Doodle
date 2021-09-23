//
// Created by TD on 2021/9/23.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
namespace doodle {

class attribute_factory_interface : public details::no_copy {
 public:
  virtual void render() = 0;

  virtual void show_attribute(const EpisodesPtr in) = 0;
  virtual void show_attribute(const ProjectPtr in) = 0;
  virtual void show_attribute(const ShotPtr in) = 0;
  virtual void show_attribute(const AssetsPtr in) = 0;
  virtual void show_attribute(const AssetsFilePtr in) = 0;
  virtual void show_attribute(const season_ptr in) = 0;

};

class attr_project : public attribute_factory_interface {
  ProjectPtr p_prj;

 public:
  attr_project();

  void set_project(const ProjectPtr& in_prj);
  virtual void render() override;
};

}  // namespace  doodle
