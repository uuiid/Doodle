//
// Created by TD on 2022/5/7.
//

#pragma once

#include <doodle_lib/gui/widgets/assets_filter_widgets/filter_base.h>
#include <doodle_lib/gui/widgets/assets_filter_widgets/filter_factory_base.h>

namespace doodle::gui {

template <class T>
class filter : public filter_base {
 public:
  T p_data;
  explicit filter(T in_t) : p_data(std::move(in_t)){};
  bool operator()(const entt::handle& in) const override { return in.all_of<T>() && in.get<T>() == p_data; }
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

 public:
  filter_factory_t() = default;
};
}  // namespace doodle::gui
