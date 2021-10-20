//
// Created by TD on 2021/10/20.
//

#include "doodle_app_base.h"

#include <doodle_lib/core/program_options.h>
namespace doodle {

doodle_app_base::doodle_app_base()
    : p_program_options(new_object<program_options>()) {
  
}
std::int32_t doodle_app_base::run() {
  return 0;
}

}  // namespace doodle
