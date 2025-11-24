//
// Created by TD on 25-5-13.
//
#include <boost/numeric/conversion/cast.hpp>

#include "local.h"
#include <cache.hpp>
#include <cache_policy.hpp>
#include <lru_cache_policy.hpp>
#include <opencv2/freetype.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>
#include <string>
#include <utility>
#include <vector>

namespace doodle::http::local {

struct video_thumbnail_cache : caches::fixed_sized_cache<uuid, std::vector<uchar>, caches::LRUCachePolicy> {
  video_thumbnail_cache() : caches::fixed_sized_cache<uuid, std::vector<uchar>, caches::LRUCachePolicy>(1) {}
};

static constexpr uuid video_thumbnail_cache_id{};

struct video_thumbnail_arg_t {
  FSys::path video_path_;
  std::double_t time_;
  // form json
  friend void from_json(const nlohmann::json& j, video_thumbnail_arg_t& p) {
    j["video_path"].get_to(p.video_path_);
    j["time"].get_to(p.time_);
  }
};
boost::asio::awaitable<boost::beast::http::message_generator> video_thumbnail::post(session_data_ptr in_handle) {
  auto l_arg = in_handle->get_json().get<video_thumbnail_arg_t>();
  if (!exists(l_arg.video_path_))
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "视频文件不存在"});
  cv::VideoCapture l_video{};
  if (!l_video.open(l_arg.video_path_.generic_string())) throw_exception(doodle_error{"mp4 打开失败"});

  auto l_video_count = l_video.get(cv::CAP_PROP_FRAME_COUNT);
  cv::Mat l_image{};
  l_video.set(
      cv::CAP_PROP_POS_FRAMES,
      std::clamp(std::clamp(l_arg.time_, std::double_t{0}, std::double_t{1}) * l_video_count, 0.0, l_video_count - 1)
  );
  l_video >> l_image;
  if (l_image.empty()) throw_exception(doodle_error{"图片解码失败"});

  // if (l_image.cols > 1920 || l_image.rows > 1080) {
  //   auto l_resize = std::min(1920.0 / l_image.cols, 1080.0 / l_image.rows);
  //   cv::resize(l_image, l_image, cv::Size{}, l_resize, l_resize);
  // }
  std::vector<uchar> l_buffer{};
  cv::imencode(".png", l_image, l_buffer, {cv::IMWRITE_PNG_BILEVEL, 0});
  g_ctx().get<video_thumbnail_cache>().Put(video_thumbnail_cache_id, l_buffer);
  co_return in_handle->make_msg(std::move(l_buffer), "image/png");
}
void video_thumbnail::init_ctx() {
  static std::once_flag l_flag{};
  std::call_once(l_flag, []() { g_ctx().emplace<video_thumbnail_cache>(); });
}

boost::asio::awaitable<boost::beast::http::message_generator> video_thumbnail::get(session_data_ptr in_handle) {
  auto& l_cache = g_ctx().get<video_thumbnail_cache>();
  if (l_cache.Cached(video_thumbnail_cache_id)) {
    auto l_buffer = *l_cache.Get(video_thumbnail_cache_id);
    co_return in_handle->make_msg(std::move(l_buffer), "image/png");
  }
  throw_exception(http_request_error{boost::beast::http::status::bad_request, "视频文件不存在"});
}

struct tools_add_watermark_arg_t {
  std::vector<FSys::path> image_paths_;
  FSys::path out_path_;
  std::string watermark_text_{"水印示例"};
  // 水印颜色
  std::string watermark_color_                          = "#FF0000";
  // 水印透明度 0~1
  std::double_t watermark_opacity_                      = 0.5;
  // 水印大小
  std::pair<std::int32_t, std::int32_t> watermark_size_ = {120, 210};
  // 水印高度
  std::int32_t watermark_height_                        = 30;
  // form json
  friend void from_json(const nlohmann::json& j, tools_add_watermark_arg_t& p) {
    j["image_paths"].get_to(p.image_paths_);
    j["out_path"].get_to(p.out_path_);
    if (j.contains("watermark_text")) j["watermark_text"].get_to(p.watermark_text_);
    if (j.contains("watermark_color")) j["watermark_color"].get_to(p.watermark_color_);
    if (j.contains("watermark_opacity")) j["watermark_opacity"].get_to(p.watermark_opacity_);
    if (j.contains("watermark_size")) j["watermark_size"].get_to(p.watermark_size_);
    if (j.contains("watermark_height")) j["watermark_height"].get_to(p.watermark_height_);
    p.watermark_opacity_ = std::clamp(p.watermark_opacity_, 0.0, 1.0);
  }
};

boost::asio::awaitable<boost::beast::http::message_generator> tools_add_watermark::post(session_data_ptr in_handle) {
  auto l_args = in_handle->get_json().get<tools_add_watermark_arg_t>();

  if (!FSys::exists(l_args.out_path_)) FSys::create_directories(l_args.out_path_);

  cv::Ptr<cv::freetype::FreeType2> const l_ft2{cv::freetype::createFreeType2()};
  l_ft2->loadFontData(std::string{doodle_config::font_default}, 0);

  constexpr std::int32_t l_thickness = -1;
  std::int32_t l_baseline            = 0;
  auto l_text_size   = l_ft2->getTextSize(l_args.watermark_text_, l_args.watermark_height_, l_thickness, &l_baseline);
  cv::Scalar l_color = cv::Scalar::all(255);
  if (!l_args.watermark_color_.empty() && l_args.watermark_color_.front() == '#' &&
      l_args.watermark_color_.length() == 7) {
    l_color = cv::Scalar{
        boost::numeric_cast<std::double_t>(std::stoi(l_args.watermark_color_.substr(1, 2), nullptr, 16)),
        boost::numeric_cast<std::double_t>(std::stoi(l_args.watermark_color_.substr(3, 2), nullptr, 16)),
        boost::numeric_cast<std::double_t>(std::stoi(l_args.watermark_color_.substr(5, 2), nullptr, 16))
    };
  }

  for (auto&& l_image_path : l_args.image_paths_) {
    if (!exists(l_image_path))
      throw_exception(
          http_request_error{
              boost::beast::http::status::bad_request, fmt::format("图片文件不存在: {}", l_image_path.generic_string())
          }
      );
    auto l_image = cv::imread(l_image_path.generic_string());
    if (l_image.empty()) continue;
    // 添加水印
    auto l_cv     = cv::imread(l_image_path.generic_string(), cv::IMREAD_UNCHANGED);
    auto l_cv_old = l_image.clone();
    /// 循环图片宽高, 添加水印
    for (std::int32_t y = 0; y < l_image.rows; y += (l_text_size.height + l_args.watermark_size_.first)) {
      for (std::int32_t x = 0; x < l_image.cols; x += (l_text_size.width + l_args.watermark_size_.second)) {
        cv::Point l_textOrg(x, y + l_text_size.height);
        l_ft2->putText(
            l_image, l_args.watermark_text_, l_textOrg, l_args.watermark_height_, l_color, l_thickness,
            cv::LineTypes::LINE_AA, true
        );
      }
    }
    /// 混合图片
    cv::addWeighted(l_image, l_args.watermark_opacity_, l_cv_old, 1 - l_args.watermark_opacity_, 0, l_image);

    cv::imwrite((l_args.out_path_ / l_image_path.filename()).generic_string(), l_image);
  }
  co_return in_handle->make_msg_204();
}

}  // namespace doodle::http::local