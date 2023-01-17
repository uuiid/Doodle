//
// Created by TD on 2022/3/24.
//

#include <doodle_core/core/doodle_lib.h>

#include "doodle_app/app/authorization.h"

int main(int argc, char *argv[]) {
  doodle::doodle_lib l_lib{};

  if (argc > 1) {
    doodle::authorization::generate_token(argv[1]);
  } else {
    doodle::authorization::generate_token(doodle::FSys::current_path());
  }
  return 0;
}
