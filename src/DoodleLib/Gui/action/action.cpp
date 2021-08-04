//
// Created by TD on 2021/6/17.
//

#include "action.h"

#include <DoodleLib/Exception/Exception.h>
#include <Logger/Logger.h>
#include <threadPool/long_term.h>

#include <boost/numeric/conversion/cast.hpp>

namespace doodle {
action_base::action_base()
    : p_name(),
      p_term(),
      _mutex() {}

std::string action_base::class_name() {
  return p_name;
}

bool action_base::is_async() {
  return false;
}

long_term_ptr action_base::get_long_term_signal() const {
  return p_term;
}

action::action()
    : action_base() {
}

long_term_ptr action::operator()(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  return run(in_data, in_parent);
}

actn_null::actn_null() {
  p_name = "取消";
}
}  // namespace doodle
