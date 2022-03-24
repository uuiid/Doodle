//
// Created by TD on 2022/3/24.
//

#include <doodle_lib/core/authorization.h>
int main(int argc, char *argv[]) {
  if (argc > 1) {
    doodle::authorization::generate_token(argv[1]);
  } else {
    doodle::authorization::generate_token(doodle::FSys::current_path());
  }
  return 0;
}
