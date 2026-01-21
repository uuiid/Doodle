#include "doodle_core/metadata/image_size.h"
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/preview_file.h>

#include <opencv2/opencv.hpp>

namespace doodle::http::preview {

struct video_info_t {
  std::double_t duration_{0.0};
  cv::Size size_{0, 0};
};

video_info_t get_video_duration(const FSys::path& in_path);
std::tuple<cv::Size, double, FSys::path> handle_video_file(
    const FSys::path& in_path, const std::size_t& in_fps, const cv::Size& in_size,
    const std::shared_ptr<preview_file>& in_preview_file
);
}  // namespace doodle::http::preview