//
// Created by TD on 2021/5/26.
//

#include "util.h"

namespace doodle::details {
identifier::identifier() : id_(0) {}
identifier::~identifier() = default;

std::uint64_t identifier::id() const { return ++id_; }

}  // namespace doodle::core
