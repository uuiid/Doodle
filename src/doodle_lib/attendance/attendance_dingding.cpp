//
// Created by TD on 2022/10/21.
//

#include "attendance_dingding.h"

namespace doodle {
namespace business {

class attendance_dingding::impl {
 public:
};

attendance_dingding::attendance_dingding() : ptr(std::make_unique<impl>()) {}
attendance_dingding::~attendance_dingding() {}
}  // namespace business
}  // namespace doodle