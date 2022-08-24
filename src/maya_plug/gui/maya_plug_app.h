//
// Created by TD on 2021/10/14.
//

#pragma once
#include <doodle_lib/app/app.h>

namespace doodle::maya_plug {
class maya_plug_app : public app {
 protected:
 public:
  maya_plug_app(const win::wnd_instance& in_instance = nullptr,
                const win::wnd_handle& in_parent     = nullptr);

 private:
  virtual void load_windows() override;

 protected:
  virtual void post_constructor() override;

 public:
 public:
  using app::app;
  virtual void close_windows() override;
};
}  // namespace doodle::maya_plug
