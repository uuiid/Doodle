#include <corelib/FileWarp/ImageSequence.h>
#include <corelib/Exception/Exception.h>

#include <opencv2/opencv.hpp>
#include <pinyinlib/convert.h>

namespace doodle {
std::string ImageSequence::clearString(const std::string &str) {
  auto &con  = dopinyin::convert::Get();
  auto str_r = std::string{};
  con.toEn(str);

  return str_r;
}

ImageSequence::ImageSequence(decltype(p_paths) paths, decltype(p_Text) text)
    : p_paths(std::move(paths)),
      p_Text(std::move(clearString(text))) {
}

ImageSequence::ImageSequence(FSys::path path_dir, decltype(p_Text) text)
    : p_paths(),
      p_Text(std::move(clearString(text))) {
  if (!FSys::is_directory(path_dir)) throw FileError{path_dir, "file not is a directory"};

  FSys::path ex{};
  for (auto &path : FSys::directory_iterator(path_dir)) {
    if (path.is_regular_file()) {
      if (ex.empty()) {
        ex = path.path().extension();
        p_paths.emplace_back(path.path());
      } else {
        if (path.path().extension() == ex) {
          p_paths.emplace_back(path.path());
        }
      }
    }
  }
}

bool ImageSequence::hasSequence() {
  return !p_paths.empty();
}

void ImageSequence::setText(const std::string &text) {
  p_Text = clearString(text);
}

void ImageSequence::createVideoFile(const FSys::path &out_file) {
  if (!this->hasSequence()) throw std::runtime_error{"not Sequence"};

  const static cv::Size k_size{1280, 720};

  auto video           = cv::VideoWriter{out_file.generic_string(),
                               cv::VideoWriter::fourcc('D', 'I', 'V', 'X'),
                               25,
                               cv::Size(1280, 720)};
  auto k_image         = cv::Mat{};
  auto k_image_resized = cv::Mat{};
  for (auto &&path : p_paths) {
    k_image = cv::imread(path.generic_string());
    if (k_image.empty())
      throw std::runtime_error("open cv not read image");
    if (k_image.cols != 1280 || k_image.rows != 720)
      cv::resize(k_image, k_image_resized, k_size);
    else
      k_image_resized = k_image;
    if (!p_Text.empty()) {
      cv::putText(k_image_resized, p_Text, cv::Point{80, 80}, cv::HersheyFonts::FONT_HERSHEY_TRIPLEX, double{1}, cv::Scalar{0, 0, 1});
    }
    video << k_image_resized;
  }
}

}  // namespace doodle