#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <boost/uuid/uuid.hpp>
namespace doodle {

class kitsu_file_path {
 public:
  kitsu_file_path()  = default;
  ~kitsu_file_path() = default;

  boost::uuids::uuid id_;
  FSys::path path_{};

};
}  // namespace doodle