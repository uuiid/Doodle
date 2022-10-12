//
// Created by TD on 2022/3/24.
//

#include <doodle_lib/core/authorization.h>
#include <doodle_core/core/doodle_lib.h>
int main(int argc, char *argv[]) {
  doodle::doodle_lib l_lib{};
  doodle::doodle_lib::create_time_database();
  if (argc > 1) {
    doodle::authorization::generate_token(argv[1]);
  } else {
    doodle::authorization::generate_token(doodle::FSys::current_path());
  }
  return 0;
}
