//
// Created by TD on 2021/12/27.
//

#include "image_to_move.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/shot.h>
#include <doodle_core/metadata/user.h>

#include "opencv2/core.hpp"
#include <opencv2/freetype.hpp>
#include <opencv2/opencv.hpp>
#include <utility>

namespace doodle {
namespace detail {
namespace {
// 计算baseline大小
void watermark_add_image(cv::Mat& in_image, const std::vector<image_to_move::image_watermark>& in_watermark) {
  auto l_string_s = in_watermark |
                    ranges::views::transform([](const image_to_move::image_watermark& in) { return in.text_attr; }) |
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

FSys::path image_to_move::create_out_path(const entt::handle& in_handle) {
  boost::ignore_unused(this);

  FSys::path l_out = in_handle.get<out_file_path>().path;
  l_out            = detail::create_out_path(
      l_out, in_handle.all_of<episodes>() ? in_handle.get<episodes>() : episodes{},
      in_handle.all_of<shot>() ? in_handle.get<shot>() : shot{}, in_handle.try_get<project>()
  );
  in_handle.replace<out_file_path>(l_out);
  return l_out;
}

FSys::path create_out_path(
    const FSys::path& in_dir, const episodes& in_eps, const shot& in_shot, const project* in_project
) {
  FSys::path l_out = in_dir;

  /// \brief 这里我们检查 shot，episode 进行路径的组合
  if (!l_out.has_extension()) {
    if (!in_project)
      l_out /= fmt::format("EP{:03}_SC{:03}{}.mp4", in_eps.p_episodes, in_shot.p_shot, in_shot.p_shot_enum);
    else
      l_out /= fmt::format(
          "{}_EP{:03}_SC{:03}{}.mp4", in_project->p_shor_str, in_eps.p_episodes, in_shot.p_shot, in_shot.p_shot_enum
      );
  } else
    l_out.extension().replace_extension(".mp4");

  if (exists(l_out.parent_path())) create_directories(l_out.parent_path());
  return l_out;
}

boost::system::error_code create_move(
    const FSys::path& in_out_path, logger_ptr in_logger, const std::vector<movie::image_attr>& in_vector
) {
  /// \brief 这里排序组件
  auto l_vector = in_vector;
  movie::image_attr::extract_num(l_vector);
  std::sort(l_vector.begin(), l_vector.end());
  std::atomic_bool l_stop{};
  boost::system::error_code l_ec{};
  // todo: 这里我们需要一个信号来停止

  in_logger->info("开始创建视频 {}", in_out_path);
  in_logger->info("获得图片路径 {}", l_vector.front().path_attr.parent_path());

  if (FSys::exists(in_out_path)) {
    FSys::remove(in_out_path, l_ec);
    if (l_ec) {
      in_logger->log(log_loc(), level::err, "合成视频主动删除失败 {} ", in_out_path);
      return l_ec;
    }
  }

  const static cv::Size k_size{1920, 1080};
  auto video   = cv::VideoWriter{in_out_path.generic_string(), cv::VideoWriter::fourcc('m', 'p', '4', 'v'), 25, k_size};
  auto k_image = cv::Mat{};
  const auto& k_size_len = l_vector.size();
  auto l_gamma           = create_gamma_LUT_table(l_vector.empty() ? 1.0 : l_vector.front().gamma_t);
  for (auto& l_image : l_vector) {
    if (l_stop) {
      in_logger->error("合成视频被主动结束 合成视频文件将被主动删除");
      try {
        remove(in_out_path);
      } catch (const FSys::filesystem_error& err) {
        in_logger->error("合成视频主动删除失败 {} ", boost::diagnostic_information(err));
      }
      in_logger->log(level::off, magic_enum::enum_name(process_message::state::fail));
      l_ec.assign(boost::system::errc::operation_canceled, boost::system::generic_category());
      BOOST_ASIO_ASSERT(l_ec);
      return l_ec;
    }

    in_logger->info("开始读取图片 {}", l_image.path_attr);

    k_image = cv::imread(l_image.path_attr.generic_string());
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
  in_logger->log(level::off, magic_enum::enum_name(process_message::state::success));
  return l_ec;
}
} // namespace detail
} // namespace doodle