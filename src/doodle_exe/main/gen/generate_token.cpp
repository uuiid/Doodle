//
// Created by TD on 2022/3/24.
//

#include <doodle_lib/core/authorization.h>
#include <doodle_lib/core/app_command_base.h>
int main(int argc, char *argv[]) {
  doodle::app_command_base l_command_base{
      doodle::app_command_base::in_app_args{nullptr, std::vector<std::string>{}}};
  for (int l_i = 0; l_i < 10; ++l_i) {
    l_command_base.poll_one();
  }
  if (argc > 1) {
    doodle::authorization::generate_token(argv[1]);
  } else {
    doodle::authorization::generate_token(doodle::FSys::current_path());
  }
  return 0;
}
