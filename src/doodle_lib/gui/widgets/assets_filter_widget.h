//
// Created by TD on 2021/9/16.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/gui_ref/base_window.h>
#include <doodle_lib/gui/gui_ref/ref_base.h>
#include <doodle_lib/core/init_register.h>

#include <boost/signals2.hpp>
namespace doodle {

namespace gui {
class DOODLELIB_API filter_base {
 public:
  virtual ~filter_base()                                = default;
  virtual bool operator()(const entt::handle& in) const = 0;
};
class DOODLELIB_API filter_factory_base {
  class impl;
  std::unique_ptr<impl> p_i;

  void connection_sig();

 protected:
  virtual std::unique_ptr<filter_base> make_filter_() = 0;
  virtual void refresh_()                             = 0;
  virtual void init()                                 = 0;

  bool is_disabled;

 public:
  filter_factory_base();
  virtual ~filter_factory_base();
  entt::observer p_obs;
  bool is_edit;
  virtual bool render() = 0;
  virtual void refresh(bool force);
  std::unique_ptr<filter_base> make_filter();
};

template <class T>
class filter : public filter_base {
 public:
  T p_data;
  explicit filter(T in_t) : p_data(std::move(in_t)){};
  bool operator()(const entt::handle& in) const override {
    return in.all_of<T>() && in.get<T>() == p_data;
  }
};

template <class T>
class filter_factory_t : public filter_factory_base {
 public:
  using data_type = T;
  using gui_cache = gui_cache<data_type>;

  std::optional<gui_cache> p_cur_select;
  std::string select_name;
  std::vector<gui_cache> p_edit;

 protected:
  std::unique_ptr<filter_base> make_filter_() override {
    if (p_cur_select)
      return std::make_unique<filter<data_type>>(p_cur_select->data);
    else
      return {};
  }
  void init() override {
    for (auto&& [e, i] : g_reg()->view<data_type>().each()) {
      p_edit.emplace_back(i);
    }
    select_name.clear();
  }
  void refresh_() override {
    for (auto&& i : p_obs) {
      auto k_h = make_handle(i);
      p_edit.emplace_back(k_h.template get<data_type>());
    }
    p_edit |= ranges::actions::sort | ranges::actions::unique;
  }

 public:
  filter_factory_t()
      : p_cur_select(),
        select_name(),
        p_edit() {
    p_obs.connect(*g_reg(), entt::collector.update<data_type>());
  }
};

}  // namespace gui

/**
 * @brief 资产显示树
 * @li 这里只显示资产树, 可以类比为文件夹树
 *
 */
class DOODLELIB_API assets_filter_widget
    : public gui::window_panel {
  class impl;
  std::unique_ptr<impl> p_impl;

  void refresh_(bool force);

 public:
  assets_filter_widget();
  ~assets_filter_widget() override;

  constexpr static std::string_view name{"过滤"};

  void init() override;
  void failed() override;
  void render() override;

  void refresh(bool force);
};

namespace assets_filter_widget_ns {
constexpr auto init = []() {
  entt::meta<assets_filter_widget>()
      .type()
      .prop("name"_hs, std::string{assets_filter_widget::name})
      .base<gui::window_panel>();
};
class init_class
    : public init_register::registrar_lambda<init, 3> {};
}  // namespace assets_filter_widget_ns

}  // namespace doodle
