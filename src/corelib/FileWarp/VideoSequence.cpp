#include <corelib/FileWarp/VideoSequence.h>

#include <opencv2/opencv.hpp>
namespace doodle {
VideoSequence::VideoSequence(decltype(p_paths) paths)
    : p_paths(std::move(paths)) {
}

void VideoSequence::connectVideo(const FSys::path& out_path) {
  if (!FSys::exists(out_path.parent_path()))
    FSys::create_directories(out_path.parent_path());

  auto k_voide_input   = cv::VideoCapture{};
  auto k_voide_out     = cv::VideoWriter{out_path.generic_string(),
                                     cv::VideoWriter::fourcc('D', 'I', 'V', 'X'),
                                     25,
                                     cv::Size(1280, 720)};
  auto k_image         = cv::Mat{};
  auto k_image_resized = cv::Mat{};
  const static cv::Size k_size{1280, 720};
  for (auto path : p_paths) {
    if (k_voide_input.open(path.generic_string())) {
      while (k_voide_input.read(k_image)) {
        if (k_image.cols != 1280 || k_image.rows != 720)
          cv::resize(k_image, k_image_resized, k_size);
        else
          k_image_resized = k_image;

        k_voide_out << k_image_resized;
      }
    }
  }
}
}  // namespace doodle