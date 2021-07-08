//
// Created by TD on 2021/7/6.
//

#include "actn_image_and_movie.h"

#include <FileWarp/ImageSequence.h>
#include <core/CoreSet.h>

namespace doodle {
actn_image_to_movie::actn_image_to_movie() {
  p_name = "转换视频";
}

bool actn_image_to_movie::is_accept(const arg_& in_any) {
  const static std::vector<FSys::path> static_path_list{
      ".exr",
      ".jpg",
      ".png"};
  try {
    auto k_path = in_any.image_list.front();
    bool is_ok  = true;
    std::vector<FSys::path> k_r{};
    if (FSys::is_directory(k_path)) {
      FSys::path k_ex{};
      std::copy_if(
          FSys::directory_iterator(k_path), FSys::directory_iterator{},
          std::inserter(k_r, k_r.begin()), [](const FSys::path& in_) {
            if (FSys::is_regular_file(in_))
              return (std::find(static_path_list.begin(), static_path_list.end(), in_.extension()) == static_path_list.end());
            else
              return false;
          });

      is_ok = k_r.empty();
    }

    if(is_ok){
      
    }

    return is_ok;
  } catch (const std::bad_cast& err) {
    DOODLE_LOG_WARN("无法转换any ", err.what())
  }
  return false;
}

void actn_image_to_movie::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  auto k_path = sig_get_arg().value();

  ImageSequence k_image{k_path.image_list.front(), CoreSet::getSet().getUser_en()};

}

}  // namespace doodle
