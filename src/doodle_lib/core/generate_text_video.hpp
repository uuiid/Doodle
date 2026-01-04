#pragma once
#include <doodle_core/core/file_sys.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/image_size.h>

#include <opencv2/core.hpp>
#include <opencv2/core/types.hpp>

namespace doodle {

class generate_text_video {
 public:
  struct font_attr_t {
    FSys::path font_path_{doodle_config::font_default};
    int font_size_{48};
    // (0,0) 为左上角, 为(0,0)时,居中而不是左上角
    cv::Point font_point_{0, 0};
    cv::Scalar font_color_{255, 255, 255, 255};
    std::string text_{};
  };
  std::vector<font_attr_t> font_attrs_{};
  FSys::path out_path_{};
  image_size size_{1280, 720};
  chrono::seconds duration_{chrono::seconds{3}};
  void add_font_attr(const font_attr_t& in_attr) { font_attrs_.emplace_back(in_attr); }
  void set_out_path(const FSys::path& in_out_path) { out_path_ = in_out_path; }
  void set_size(const image_size& in_size) { size_ = in_size; }
  void set_duration(const chrono::seconds& in_duration) { duration_ = in_duration; }

  void run();
};
}  // namespace doodle