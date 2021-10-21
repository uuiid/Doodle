//
// Created by TD on 2021/9/15.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/lib_warp/imgui_warp.h>

#include <boost/hana/experimental/printable.hpp>
namespace doodle {
class DOODLELIB_API base_widget
    : public details::no_copy,
      public std::enable_shared_from_this<base_widget> {
 protected:
  string p_class_name;

  virtual bool use_register() { return true; };

 public:
  virtual void post_constructor();
  virtual void frame_render() = 0;
  virtual const string& get_class_name() const;
};

class DOODLELIB_API metadata_widget : public base_widget {
 protected:
  attribute_factory_ptr p_factory;

 public:
  virtual attribute_factory_ptr get_factory();
};

class DOODLELIB_API windows_warp_base : public base_widget {
 public:
  bool_ptr p_show;
  windows_warp_base(bool init_show = false)
      : p_show(new_object<bool>(init_show)){};
  virtual bool load_show()                                = 0;
  virtual void save_show() const                          = 0;
  virtual std::shared_ptr<base_widget> get_widget() const = 0;
};

template <class widget>
class DOODLELIB_API windows_warp : public windows_warp_base {
 public:
  using widget_ptr = std::shared_ptr<widget>;

  widget_ptr p_widget;

  windows_warp(bool init_show = false)
      : windows_warp_base(init_show),
        p_widget(new_object<widget>()){};

  void frame_render() override {
    if (*p_show) {
      dear::Begin{
          this->p_widget->get_class_name().c_str(),
          p_show.get()} &&
          std::bind(&widget::frame_render, this->p_widget.get());
    }
  }
  const string& get_class_name() const override {
    return this->p_widget->get_class_name();
  }

  bool load_show() override {
    auto& set = core_set::getSet();
    if (set.widget_show.count(this->get_class_name()) > 0) {
      *p_show = set.widget_show[this->get_class_name()];
      return true;
    }
    return false;
  }
  void save_show() const override {
    auto& set                               = core_set::getSet();
    set.widget_show[this->get_class_name()] = *p_show;
  }
  std::shared_ptr<base_widget> get_widget() const override {
    return p_widget;
  }
};

template <class widget>
std::shared_ptr<windows_warp<widget>> win_warp_cast(const base_widget_ptr& in) {
  return std::dynamic_pointer_cast<windows_warp<widget>>(in);
}

template <class widget>
std::shared_ptr<widget> win_cast(const base_widget_ptr& in) {
  auto ptr = win_warp_cast<widget>(in);
  if (ptr)
    return ptr->p_widget;
  else
    return nullptr;
}

}  // namespace doodle
