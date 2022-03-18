//
// Created by TD on 2022/1/20.
//

#pragma once
#include <doodle_lib/gui/gui_ref/ref_base.h>

namespace doodle::gui {

class DOODLELIB_API project_edit : public edit_interface {
  class impl;
  std::unique_ptr<impl> p_i;

  void init_(const entt::handle &in) override;
  void save_(const entt::handle &in) const override;

 public:
  project_edit();
  ~project_edit();
  void render(const entt::handle &in) override;
};

class DOODLELIB_API base_config_edit : public edit_interface {
  class impl;
  std::unique_ptr<impl> p_i;
  void init_(const entt::handle &in) override;
  void save_(const entt::handle &in) const override;

  void set_config_init(const entt::handle &in, const std::string &in_name);

 public:
  base_config_edit();
  ~base_config_edit();
  void render(const entt::handle &in) override;
};

// class DOODLELIB_API modle_config_edit : public edit_interface{
//  private:
//   class impl;
//   std::unique_ptr<impl> p_i;
//  public:
//   modle_config_edit();
//   virtual ~modle_config_edit();
//   virtual void render(const entt::handle &in) override;

//  protected:
//   virtual void init_(const entt::handle &in) override;
//   virtual void save_(const entt::handle &in) const override;
// };
}  // namespace doodle::gui
