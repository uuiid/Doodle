#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/preview_file.h>

#include <opencv2/opencv.hpp>


namespace doodle::http::preview {

std::tuple<cv::Size, double, FSys::path> get_handle_video_file(
    const FSys::path& in_path, const uuid& in_id, const std::size_t& in_fps, const cv::Size& in_size
);
std::tuple<cv::Size, double, FSys::path> handle_video_file(
    const FSys::path& in_path, const uuid& in_id, const std::size_t& in_fps, const cv::Size& in_size,
    const std::shared_ptr<preview_file>& in_preview_file
);
}  // namespace doodle::http::preview