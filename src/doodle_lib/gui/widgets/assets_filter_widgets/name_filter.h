//
// Created by TD on 2022/5/7.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/widgets/assets_filter_widgets/filter_base.h>

namespace doodle {
namespace gui {

class DOODLELIB_API name_filter : public filter_base {
 private:
  std::string name_;

 public:
  explicit name_filter(std::string in_name) : name_(std::move(in_name)) {}
  virtual bool operator()(const entt::handle& in) const override;
};

}  // namespace gui
}  // namespace doodle
