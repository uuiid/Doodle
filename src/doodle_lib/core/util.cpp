//
// Created by TD on 2021/5/26.
//

#include "util.h"

#include <doodle_lib/gui/strand_gui.h>
namespace doodle::core {
identifier::identifier()
    : id_(0) {}
identifier::~identifier() = default;

identifier& identifier::get() {
  static identifier l_{};
  return l_;
}
std::uint64_t identifier::id() {
  return ++id_;
}

}  // namespace doodle::core

