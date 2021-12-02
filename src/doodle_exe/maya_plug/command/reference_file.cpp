//
// Created by TD on 2021/11/30.
//

#include "reference_file.h"
namespace doodle::maya_plug {
reference_file::reference_file()
    : prj_ref(boost::uuids::nil_uuid()),
      path(),
      use_sim(),
      high_speed_sim(),
      collision_ref_file(),
      collision_model(){};

reference_file::reference_file(const uuid &in_uuid)
    : reference_file() {
  prj_ref = in_uuid;
}

}  // namespace doodle::maya_plug
