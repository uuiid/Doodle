//
// Created by TD on 2022/4/21.
//

#include "subtitle_processing.h"

namespace doodle::gui {
class subtitle_processing::impl {
 public:
  impl() = default;
};

subtitle_processing::subtitle_processing()
    : p_i(std::make_unique<impl>()) {
}
void subtitle_processing::render() {
}
subtitle_processing::~subtitle_processing() = default;
}  // namespace doodle::gui
