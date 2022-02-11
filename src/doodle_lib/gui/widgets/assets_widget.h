//
// Created by TD on 2021/9/16.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/base_windwos.h>

#include <boost/signals2.hpp>
namespace doodle {

namespace gui {
class DOODLELIB_API filter_base {
 public:
  virtual bool operator()(const entt::handle& in) const = 0;
};
class DOODLELIB_API filter_factory_base {
 protected:
  virtual std::unique_ptr<filter_base> make_filter_() = 0;

 public:
  entt::observer p_obs;
  virtual bool render() = 0;
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
}  // namespace gui

/**
 * @brief 资产显示树
 * @image html attr_project.jpg 资产树
 * @li 这里只显示资产树, 可以类比为文件夹树
 *
 */
class DOODLELIB_API assets_widget : public process_t<assets_widget> {
  class impl;
  std::unique_ptr<impl> p_impl;

 public:
  assets_widget();
  ~assets_widget() override;
  /**
   * @brief 设置这个小部件要创建的树的根属性
   *
   * @param in_entity
   */
  void set_metadata(const entt::entity& in_entity);

  constexpr static std::string_view name{"资产"};

  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(delta_type, void* data);

  boost::signals2::signal<void(const entt::handle&)> select_change;
};
}  // namespace doodle
