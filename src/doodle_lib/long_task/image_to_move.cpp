//
// Created by TD on 2021/12/27.
//

#include "image_to_move.h"

#include "doodle_core/exception/exception.h"
#include <doodle_core/core/core_set.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/shot.h>
#include <doodle_core/metadata/user.h>

#include "opencv2/core.hpp"
#include <opencv2/freetype.hpp>
#include <opencv2/opencv.hpp>
#include <spdlog/spdlog.h>
#include <utility>

namespace doodle {
namespace detail {
namespace {
// 计算baseline大小
void watermark_add_image(cv::Mat& in_image, const std::vector<::doodle::movie::image_watermark>& in_watermark) {
  auto l_string_s = in_watermark |
                    ranges::views::transform([](const ::doodle::movie::image_watermark& in) { return in.text_attr; }) |
                    ranges::to_vector;
  cv::Ptr<cv::freetype::FreeType2> const l_ft2{cv::freetype::createFreeType2()};
  l_ft2->loadFontData(std::string{doodle_config::font_default}, 0);

  std::int32_t l_fontHeight = 30;

  std::map<std::double_t, std::string> l_string_map{};
  std::map<std::double_t, std::int32_t> l_baseline_map{};
  for (auto&& l_watermark : in_watermark) {
    l_string_map[l_watermark.height_proportion_attr] += l_watermark.text_attr;
  }

  auto l_image                       = in_image;
  constexpr std::int32_t l_thickness = -1;
  for (auto&& [l_key, l_string] : l_string_map) {
    std::int32_t l_baseline = 0;

    auto textSize           = l_ft2->getTextSize(l_string, l_fontHeight, l_thickness, &l_baseline);
    if (l_thickness > 0) l_baseline += l_thickness;
    l_baseline_map[l_key] = l_baseline;
  }

  std::int32_t l_baseline_tmp = 0;
  for (auto&& l_watermark : in_watermark) {
    if (l_watermark.text_attr.empty()) continue;
    auto textSize = l_ft2->getTextSize(l_watermark.text_attr, l_fontHeight, l_thickness, &l_baseline_tmp);

    textSize.width += l_baseline_map[l_watermark.height_proportion_attr];
    textSize.height += l_baseline_map[l_watermark.height_proportion_attr];
    // center the text
    cv::Point textOrg(
        (in_image.cols) * l_watermark.width_proportion_attr, (in_image.rows) * l_watermark.height_proportion_attr
    );

    // draw the box
    cv::rectangle(
        l_image, textOrg + cv::Point(0, l_baseline_map[l_watermark.height_proportion_attr]),
        textOrg + cv::Point(textSize.width, -textSize.height), cv::Scalar(30, 31, 34, 75), -1
    );

    cv::addWeighted(l_image, 0.7, in_image, 0.3, 0, in_image);
    // then put the text itself
    l_ft2->putText(
        in_image, l_watermark.text_attr, textOrg, l_fontHeight,
        cv::Scalar{
            l_watermark.rgba_attr[0], l_watermark.rgba_attr[1], l_watermark.rgba_attr[2], l_watermark.rgba_attr[3]
        },
        l_thickness, cv::LineTypes::LINE_AA, true
    );
  }
}

auto create_gamma_LUT_table(const std::double_t& in_gamma) {
  cv::Mat lookupTable(1, 256, CV_8U);
  uchar* p = lookupTable.ptr();

  for (int i = 0; i < 256; ++i) p[i] = cv::saturate_cast<uchar>(std::pow(i / 255.0, in_gamma) * 255.0);
  return lookupTable;
}
}  // namespace

FSys::path create_out_path(
    const FSys::path& in_dir, const episodes& in_eps, const shot& in_shot, const std::string& in_project_short_string
) {
  FSys::path l_out = in_dir;

  /// \brief 这里我们检查 shot，episode 进行路径的组合
  if (!l_out.has_extension()) {
    if (in_project_short_string.empty())
      l_out /= fmt::format("EP{:03}_SC{:03}{}.mp4", in_eps.p_episodes, in_shot.p_shot, in_shot.p_shot_enum);
    else
      l_out /= fmt::format(
          "{}_EP{:03}_SC{:03}{}.mp4", in_project_short_string, in_eps.p_episodes, in_shot.p_shot, in_shot.p_shot_enum
      );
  } else
    l_out.extension().replace_extension(".mp4");

  if (exists(l_out.parent_path())) create_directories(l_out.parent_path());
  return l_out;
}

void create_move(
    const FSys::path& in_out_path, logger_ptr in_logger, const std::vector<movie::image_attr>& in_vector,
    const image_size& in_image_size

) {
  /// \brief 这里排序组件
  auto l_vector = in_vector;
  if (std::ranges::any_of(l_vector, [](const movie::image_attr& in_item) { return in_item.num_attr == 0; }))
    movie::image_attr::extract_num(l_vector);
  std::sort(l_vector.begin(), l_vector.end());
  in_logger->info("开始创建视频 {}", in_out_path);
  in_logger->info("获得图片路径 {}", l_vector.front().path_attr.parent_path());

  if (FSys::exists(in_out_path)) {
    FSys::remove(in_out_path);
  }
  if (in_image_size.width <= 0 || in_image_size.height <= 0) {
    cv::Mat l_image = cv::imread(l_vector.front().path_attr.generic_string());
    if (l_image.empty())
      throw_exception(doodle_error{fmt::format("{} 图片读取失败 无法获取图片尺寸", l_vector.front().path_attr)});
    SPDLOG_LOGGER_INFO(in_logger, "未指定输出尺寸，使用第一张图片尺寸 {}x{}", l_image.cols, l_image.rows);
  }
  const cv::Size k_size{in_image_size.width, in_image_size.height};
  auto video   = cv::VideoWriter{in_out_path.generic_string(), cv::VideoWriter::fourcc('a', 'v', 'c', '1'), 25, k_size};
  auto k_image = cv::Mat{};
  const auto& k_size_len = l_vector.size();
  auto l_gamma           = create_gamma_LUT_table(l_vector.empty() ? 1.0 : l_vector.front().gamma_t);
  for (auto& l_image : l_vector) {
    in_logger->info("开始读取图片 {}", l_image.path_attr);

    if (l_image.path_attr.extension() == ".exr") {
      k_image = cv::imread(l_image.path_attr.generic_string(), cv::IMREAD_UNCHANGED);
      cv::normalize(k_image, k_image, 0, 255, cv::NORM_MINMAX, CV_8U);
      if (k_image.channels() == 4) cv::cvtColor(k_image, k_image, cv::COLOR_BGRA2BGR);
    } else {
      k_image = cv::imread(l_image.path_attr.generic_string());
    }
    if (k_image.empty()) {
      DOODLE_LOG_ERROR("{} 图片读取失败 跳过", l_image.path_attr);
      continue;
    }
    if (k_image.cols != k_size.width || k_image.rows != k_size.height) cv::resize(k_image, k_image, k_size);

    if (l_image.gamma_t) {
      cv::LUT(k_image, l_gamma, k_image);
    }
    watermark_add_image(k_image, l_image.watermarks_attr);
    in_logger->info("开始写入图片 {}", l_image.path_attr);
    in_logger->info("progress 1/{}", k_size_len + k_size_len / 10);
    video << k_image;
  }

  in_logger->info("成功完成任务");
}

void image_to_move::create_out_path(
    const FSys::path& in_dir, const episodes& in_eps, const shot& in_shot, const std::string& in_project_short_string
) {
  FSys::path l_out = in_dir;

  /// \brief 这里我们检查 shot，episode 进行路径的组合
  if (!l_out.has_extension()) {
    if (in_project_short_string.empty())
      l_out /= fmt::format("EP{:03}_SC{:03}{}.mp4", in_eps.p_episodes, in_shot.p_shot, in_shot.p_shot_enum);
    else
      l_out /= fmt::format(
          "{}_EP{:03}_SC{:03}{}.mp4", in_project_short_string, in_eps.p_episodes, in_shot.p_shot, in_shot.p_shot_enum
      );
  } else
    l_out.extension().replace_extension(".mp4");

  if (exists(l_out.parent_path())) create_directories(l_out.parent_path());
  out_path_ = l_out;
}

boost::asio::awaitable<void> image_to_move::run() {
  create_move(out_path_, logger_ptr_, vector_, image_size_);
  co_return;
}

void image_to_move::set_image_attr_from_dir(const FSys::path& in_path) {
  std::vector<FSys::path> l_paths{};
  for (auto&& l_path_info : FSys::directory_iterator{in_path}) {
    auto l_ext = l_path_info.path().extension();
    if (l_ext == ".png" || l_ext == ".exr" || l_ext == ".jpg") l_paths.emplace_back(l_path_info.path());
  }
  if (l_paths.empty()) throw_exception(doodle_error{"路径下没有图片(格式必须是 .png, .jpg, .exr)"});
  auto l_images = doodle::movie::image_attr::make_default_attr(&eps_, &shot_, l_paths);
  for (auto&& l_image : l_images) {
    l_image.watermarks_attr.emplace_back(user_name_, 0.7, 0.2, movie::image_watermark::rgb_default);
  }
  set_image_attr(l_images);
}

}  // namespace detail
}  // namespace doodle