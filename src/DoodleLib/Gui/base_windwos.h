//
// Created by TD on 2021/9/15.
//

#pragma once
#include <DoodleLib/doodleLib_fwd.h>
#include <DoodleLib/libWarp/imgui_warp.h>

#include <boost/hana/experimental/printable.hpp>
namespace doodle {
class DOODLELIB_API base_widget
    : public details::no_copy,
      public std::enable_shared_from_this<base_widget> {
 protected:
  string p_class_name;

 public:
  virtual void post_constructor();
  virtual void frame_render() = 0;
  virtual const string& get_class_name();
};

class DOODLELIB_API metadata_widget : public base_widget {
 protected:
  attribute_factory_ptr p_factory;

 public:
  virtual attribute_factory_ptr get_factory();
};

template <class widget>
class DOODLELIB_API windows_warp : public base_widget {
 public:
  using widget_ptr = std::shared_ptr<widget>;

  bool_ptr p_show;
  widget_ptr p_widget;

  windows_warp(bool init_show= false)
      : p_show(new_object<bool>(init_show)),
        p_widget(new_object<widget>()){};

  void frame_render() override {
    if (*p_show) {
      dear::Begin{
          fmt::format("{}###{}",
                      this->p_widget->get_class_name(),
                      fmt::ptr(p_widget.get()))
              .c_str(),
          p_show.get()} &&
          std::bind(&widget::frame_render, this->p_widget.get());
    }
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
