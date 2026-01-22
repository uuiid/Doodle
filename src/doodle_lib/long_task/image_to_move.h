//
// Created by TD on 2021/12/27.
//
#pragma once

#include "doodle_core/metadata/image_size.h"
#include <doodle_core/metadata/move_create.h>

#include <doodle_lib/core/asyn_task.h>
#include <doodle_lib/doodle_lib_fwd.h>

#include <filesystem>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/freetype.hpp>
#include <string>
namespace doodle {
namespace detail {
FSys::path create_out_path(
    const FSys::path& in_dir, const episodes& in_eps, const shot& in_shot,
    const std::string& in_project_short_string = {}
);
void create_move(
    const FSys::path& in_out_path, logger_ptr in_msg, const std::vector<movie::image_attr>& in_vector,
    const image_size& in_image_size = image_size{}
);
image_size get_image_size(const FSys::path& in_path);

class add_watermark_t {
  cv::Ptr<cv::freetype::FreeType2> ft2_;
  static constexpr std::int32_t g_thickness = -1;
  std::int32_t baseline_                    = 0;
  cv::Size text_size_{0, 0};
  cv::Scalar color_{255, 255, 255};
  std::double_t opacity_{0.5};
  std::string watermark_text_;
  std::int32_t watermark_height_{30};
  // 水印间距
  std::pair<std::int32_t, std::int32_t> watermark_size_{100, 100};
  cv::Mat add_watermark_to_image(const cv::Mat& in_mat) const;

 public:
  explicit add_watermark_t(
      const std::string& in_watermark_text, const std::int32_t in_watermark_height,
      const std::pair<std::int32_t, std::int32_t>& in_watermark_size = {100, 100},
      const cv::Scalar& in_watermark_color = {255, 255, 255}, std::double_t in_opacity = 0.5,
      const std::string& in_font_path = {}
  );
  /**
   * @brief 给图片添加水印
   * @param in_image_path 输入图片路径
   * @param in_out_path 输出图片路径
   * @param in_size 重新缩放大小后添加水印, 保持水印比例
   */
  void operator()(
      const FSys::path& in_image_path, const FSys::path& in_out_path, const cv::Size& in_size = {0, 0}
  ) const;
  cv::Mat operator()(const cv::Mat& in_image, const cv::Size& in_size = {0, 0}) const;
};

class image_to_move : public async_task {
 public:
  FSys::path out_path_;
  image_size image_size_;
  shot shot_;
  episodes eps_;
  std::string user_name_;

  // set image attr
  void set_image_attr(const std::vector<movie::image_attr>& in_vector) { vector_ = in_vector; }
  // 从目录总设置默认的 image attr
  void set_image_attr_from_dir(const FSys::path& in_path);

  void create_out_path(
      const FSys::path& in_dir, const episodes& in_eps, const shot& in_shot,
      const std::string& in_project_short_string = {}
  );

  boost::asio::awaitable<void> run() override;

  friend void from_json(const nlohmann::json& nlohmann_json_j, image_to_move& nlohmann_json_t) {
    FSys::path path_;
    nlohmann_json_j["path"].get_to(path_);
    nlohmann_json_j["out_path"].get_to(nlohmann_json_t.out_path_);
    nlohmann_json_j["image_size"].get_to(nlohmann_json_t.image_size_);
    nlohmann_json_j["shot"].get_to(nlohmann_json_t.shot_);
    nlohmann_json_j["episodes"].get_to(nlohmann_json_t.eps_);
    nlohmann_json_j["user_name"].get_to(nlohmann_json_t.user_name_);

    nlohmann_json_t.set_image_attr_from_dir(path_);
  }

 private:
  std::vector<movie::image_attr> vector_;
};

}  // namespace detail
}  // namespace doodle