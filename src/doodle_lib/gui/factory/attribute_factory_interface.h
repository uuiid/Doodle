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

class DOODLELIB_API attr_project : public attribute_factory_interface {
  project_ptr p_prj;
  command_ptr p_comm;

 public:
  attr_project();

  void render() override;
  void show_attribute(const project_ptr& in) override;
};

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
   void show_attribute(const assets_file_ptr& in)override;

};

}  // namespace  doodle
