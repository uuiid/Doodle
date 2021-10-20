//
// Created by TD on 2021/10/20.
//

#include "doodle_app_base.h"

#include <doodle_lib/core/doodle_lib.h>

namespace doodle {

doodle_app_base::doodle_app_base()
    : p_lib(new_object<doodle_lib>()) {
}

}  // namespace doodle
