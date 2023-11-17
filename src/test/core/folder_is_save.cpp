//
// Created by TD on 2023/11/17.
//

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/logger/logger.h>
int core_folder_is_save(int argc, char* argv[]) {
  using namespace doodle;
  if (FSys::folder_is_save("//192.168.20.7/test_delte/test.txt")) {
    std::cout << "test_folder_is_save true" << std::endl;
  } else
    return 1;
  if (!FSys::folder_is_save("//192.168.20.7/test_read_write/test.txt")) {
    std::cout << "test_folder_is_save false" << std::endl;
  } else
    return 1;
  return 0;
}