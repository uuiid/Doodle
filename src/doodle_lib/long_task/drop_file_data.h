//
// Created by TD on 2022/1/28.
//
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {

class DOODLELIB_API drop_file_data : public process_t<drop_file_data> {
  std::vector<FSys::path> files_;
  bool has_files;
 public:
  class DOODLELIB_API files {
   public:
    std::vector<FSys::path> files;
  };
  drop_file_data();
  ~drop_file_data() override;

  void drag_leave();
  void set_files(const std::vector<FSys::path>& in_paths);

  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(const delta_type&, void* data);
};

}  // namespace doodle
