//
// Created by TD on 2021/7/6.
//

#include "actn_image_and_movie.h"

#include <FileWarp/ImageSequence.h>
#include <core/CoreSet.h>

namespace doodle {

bool actn_image_to_movie::is_accept(const _arg& in_any) {
  try {
    auto k_path = dynamic_cast<const arg_&>(in_any).image_list.front();
    bool is_ok  = true;

    if (FSys::is_directory(k_path)) {
      FSys::path k_ex{};
      for (auto& k_p : FSys::directory_iterator(k_path)) {
        if (!FSys::is_regular_file(k_p))
          is_ok = false;
      }
    }
    return is_ok;
  } catch (const std::bad_cast& err) {
    DOODLE_LOG_WARN("无法转换any ", err.what())
  }
  return false;
}

void actn_image_to_movie::run(const MetadataPtr& in_data) {
  auto k_path = std::any_cast<FSys::path>(sig_get_input().get());
  ImageSequence k_image{k_path, CoreSet::getSet().getUser_en()};
  // k_image.createVideoFile()
}

}  // namespace doodle
