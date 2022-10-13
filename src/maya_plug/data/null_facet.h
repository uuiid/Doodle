//
// Created by TD on 2022/10/13.
//

#include <doodle_core/core/app_facet.h>

namespace doodle {
namespace maya_plug {

class null_facet : public doodle::detail::app_facet_interface {
 public:
  const std::string& name() const noexcept override;
  void operator()() override {
  }
  void deconstruction() override {
  }
};

}  // namespace maya_plug
}  // namespace doodle
