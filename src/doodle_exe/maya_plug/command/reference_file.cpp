//
// Created by TD on 2021/11/30.
//

#include "reference_file.h"

#include <doodle_lib/metadata/metadata.h>
namespace doodle::maya_plug {
reference_file::reference_file()
    : prj_ref(boost::uuids::nil_uuid()),
      path(),
      use_sim(false),
      high_speed_sim(false),
      collision_model(){};

reference_file::reference_file(const entt::handle &in_uuid, const string &in_u8_path)
    : reference_file() {
  prj_ref = in_uuid.get<database>().uuid();
  path    = in_u8_path;
}

}  // namespace doodle::maya_plug
