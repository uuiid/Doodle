//
// Created by TD on 2022/5/7.
//
#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/widgets/assets_filter_widgets/filter_factory_base.h>
namespace doodle::gui {

class DOODLELIB_API name_filter_factory : public filter_factory_base {
 private:
  class impl;
  std::unique_ptr<impl> ptr;

 public:
  name_filter_factory();
  ~name_filter_factory() override;
  bool render() override;

 protected:
  std::unique_ptr<filter_base> make_filter_() override;
  void refresh_() override;
  void init() override;
};

}  // namespace doodle::gui
