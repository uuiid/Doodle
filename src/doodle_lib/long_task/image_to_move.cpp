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
void watermark_add_image(cv::Mat &in_image, const image_to_move::image_watermark &in_watermark) {
  auto l_image     = in_image;
  int fontFace     = cv::HersheyFonts::FONT_HERSHEY_COMPLEX;
  double fontScale = 1;
  int thickness    = -1;
  int baseline     = 0;
  int fontHeight   = 60;
  int linestyle    = 8;
  cv::Ptr<cv::freetype::FreeType2> ft2{cv::freetype::createFreeType2()};
  ft2->loadFontData(std::string{doodle_config::font_default}, 0);
  auto textSize = ft2->getTextSize(in_watermark.text_attr, fontHeight, thickness, &baseline);
  if (thickness > 0) baseline += thickness;
  textSize.width += baseline;
  textSize.height += baseline;
  // center the text
  cv::Point textOrg(
      (in_image.cols - textSize.width) * in_watermark.width_proportion_attr,
      (in_image.rows + textSize.height) * in_watermark.height_proportion_attr
  );

  // draw the box
  cv::rectangle(
      l_image, textOrg + cv::Point(0, baseline), textOrg + cv::Point(textSize.width, -textSize.height),
      cv::Scalar(0, 0, 0, 100), -1
  );

  cv::addWeighted(l_image, 0.7, in_image, 0.3, 0, in_image);
  // then put the text itself
  ft2->putText(
      in_image, in_watermark.text_attr, textOrg, fontHeight,
      cv::Scalar{
          in_watermark.rgba_attr[0], in_watermark.rgba_attr[1], in_watermark.rgba_attr[2], in_watermark.rgba_attr[3]},
      thickness, cv::LineTypes::LINE_AA, true
  );
}

auto create_gamma_LUT_table(const std::double_t &in_gamma) {
  cv::Mat lookupTable(1, 256, CV_8U);
  uchar *p = lookupTable.ptr();

  for (int i = 0; i < 256; ++i) p[i] = cv::saturate_cast<uchar>(std::pow(i / 255.0, in_gamma) * 255.0);
  return lookupTable;
}
}  // namespace

class image_to_move::impl {
 public:
  impl() = default;
};

image_to_move::~image_to_move() = default;

image_to_move::image_to_move() : p_i(std::make_unique<impl>()) {}
void image_to_move::create_move(
    const FSys::path &in_out_path, process_message &in_msg, const std::vector<image_to_move::image_attr> &in_vector
) {
  /// \brief 这里排序组件
  auto l_vector = in_vector;
  image_attr::extract_num(l_vector);
  std::sort(l_vector.begin(), l_vector.end());
  std::atomic_bool l_stop{};
  /// \brief 这里进行消息初始化
  in_msg.set_state(in_msg.run);
  boost::signals2::scoped_connection l_connection = in_msg.aborted_sig.connect(l_s = std::addressof(l_stop)]() mutable {
    if (!(*l_s)) {
      *l_s = true;
    }
  });

  in_msg.message(fmt::format("获得图片路径 {}", l_vector.front().path_attr.parent_path()));

  in_msg.message(fmt::format("开始创建视频 {}", in_out_path));
  in_msg.set_name(in_out_path.filename().generic_string());

  const static cv::Size k_size{1920, 1080};
  auto video   = cv::VideoWriter{in_out_path.generic_string(), cv::VideoWriter::fourcc('m', 'p', '4', 'v'), 25, k_size};
  auto k_image = cv::Mat{};
  const auto &k_size_len = l_vector.size();
  auto l_gamma           = create_gamma_LUT_table(l_vector.empty() ? 1.0 : l_vector.front().gamma_t);
  for (auto &l_image : l_vector) {
    if (l_stop) {
      in_msg.set_state(in_msg.fail);
      auto k_str = fmt::format("合成视频被主动结束 合成视频文件将被主动删除\n");
      in_msg.message(k_str, in_msg.warning);
      try {
        remove(in_out_path);
      } catch (const FSys::filesystem_error &err) {
        auto l_str = fmt::format("合成视频主动删除失败 {}\n", boost::diagnostic_information(err));
        in_msg.message(l_str, in_msg.warning);
        DOODLE_LOG_WARN(l_str);
      }
      return;
    }

    in_msg.message(fmt::format("开始读取图片 {}", l_image.path_attr));
    k_image = cv::imread(l_image.path_attr.generic_string());
    if (k_image.empty()) {
      DOODLE_LOG_ERROR("{} 图片读取失败 跳过", l_image.path_attr);
      continue;
    }
    if (k_image.cols != k_size.width || k_image.rows != k_size.height) cv::resize(k_image, k_image, k_size);

    for (auto &k_w : l_image.watermarks_attr) {
      if (l_image.gamma_t) {
        cv::LUT(k_image, l_gamma, k_image);
      }
      //      watermark_add_image(k_image, k_w);
    }
    in_msg.progress_step(rational_int{1, k_size_len});
    video << k_image;
  }

  in_msg.set_state(in_msg.success);
  auto k_str = fmt::format("成功完成任务\n");
  in_msg.message(k_str, in_msg.warning);
}
FSys::path image_to_move::create_out_path(const entt::handle &in_handle) {
  boost::ignore_unused(this);

  FSys::path l_out{};
  l_out = in_handle.get<FSys::path>();

  /// \brief 这里我们检查 shot，episode 进行路径的组合
  if (!l_out.has_extension() && in_handle.any_of<episodes, shot>())
    l_out /= fmt::format(
        "{}_{}.mp4", in_handle.any_of<episodes>() ? fmt::to_string(in_handle.get<episodes>()) : "eps_none"s,
        in_handle.any_of<shot>() ? fmt::to_string(in_handle.get<shot>()) : "sh_none"s
    );
  else if (!l_out.has_extension()) {
    l_out /= fmt::format("{}.mp4", core_set::get_set().get_uuid());
  } else
    l_out.extension() == ".mp4" ? void() : throw_exception(doodle_error{"扩展名称不是MP4"});

  if (exists(l_out.parent_path())) create_directories(l_out.parent_path());
  return l_out;
}
}  // namespace detail
}  // namespace doodle
