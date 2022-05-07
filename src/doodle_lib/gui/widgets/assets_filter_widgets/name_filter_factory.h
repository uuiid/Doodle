//
// Created by TD on 2022/5/7.
//
#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/widgets/assets_filter_widgets/filter_factory_base.h>
namespace doodle {
namespace gui {

class DOODLELIB_API name_filter_factory : public filter_factory_base {
 private:
  class impl;
  std::unique_ptr<impl> ptr;

 public:
  virtual bool render() override;

 protected:
  virtual std::unique_ptr<filter_base> make_filter_() override;
  virtual void refresh_() override;
  virtual void init() override;
};

}  // namespace gui
}  // namespace doodle
