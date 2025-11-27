//
// Created by TD on 25-5-13.
//
#include "doodle_core/core/core_set.h"
#include "doodle_core/core/file_sys.h"
#include "doodle_core/core/global_function.h"

#include <doodle_lib/core/socket_io/broadcast.h>

#include <boost/asio/post.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include "local.h"
#include <cache.hpp>
#include <cache_policy.hpp>
#include <lru_cache_policy.hpp>
#include <opencv2/core/mat.hpp>
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

  constexpr static auto g_config_name                   = "tools_add_watermark.config";
  // 图片大小
  std::pair<std::int32_t, std::int32_t> image_size_     = {1920, 1080};

  // form json
  friend void from_json(const nlohmann::json& j, tools_add_watermark_arg_t& p) {
    j["image_paths"].get_to(p.image_paths_);
    j["out_path"].get_to(p.out_path_);
    if (j.contains("watermark_text")) j["watermark_text"].get_to(p.watermark_text_);
    if (j.contains("watermark_color")) j["watermark_color"].get_to(p.watermark_color_);
    if (j.contains("watermark_opacity")) j["watermark_opacity"].get_to(p.watermark_opacity_);
    if (j.contains("watermark_size")) j["watermark_size"].get_to(p.watermark_size_);
    if (j.contains("watermark_height")) j["watermark_height"].get_to(p.watermark_height_);
    if (j.contains("image_size")) j["image_size"].get_to(p.image_size_);
    p.watermark_opacity_ = std::clamp(p.watermark_opacity_, 0.0, 1.0);
  }
  // to json
  friend void to_json(nlohmann::json& j, const tools_add_watermark_arg_t& p) {
    j["image_paths"]       = p.image_paths_;
    j["out_path"]          = p.out_path_;
    j["watermark_text"]    = p.watermark_text_;
    j["watermark_color"]   = p.watermark_color_;
    j["watermark_opacity"] = p.watermark_opacity_;
    j["watermark_size"]    = p.watermark_size_;
    j["watermark_height"]  = p.watermark_height_;
  }
};

namespace {
cv::Mat add_watermark_to_image(
    const cv::Mat& in_mat, const tools_add_watermark_arg_t& in_args, cv::Ptr<cv::freetype::FreeType2> const& in_ft2,
    const cv::Size& l_text_size, const cv::Scalar& l_color, const std::int32_t l_thickness
) {
  if (in_mat.empty()) throw_exception(doodle_error{"图片解码失败"});
  // 添加水印
  auto l_cv_old = in_mat.clone();
  /// 循环图片宽高, 添加水印
  for (std::int32_t y = 0; y < in_mat.rows; y += (l_text_size.height + in_args.watermark_size_.first)) {
    for (std::int32_t x = 0; x < in_mat.cols; x += (l_text_size.width + in_args.watermark_size_.second)) {
      cv::Point l_textOrg(x, y + l_text_size.height);
      in_ft2->putText(
          in_mat, in_args.watermark_text_, l_textOrg, in_args.watermark_height_, l_color, l_thickness,
          cv::LineTypes::LINE_AA, true
      );
    }
  }
  /// 混合图片
  cv::addWeighted(in_mat, in_args.watermark_opacity_, l_cv_old, 1 - in_args.watermark_opacity_, 0, in_mat);
  return in_mat;
}
}  // namespace

boost::asio::awaitable<boost::beast::http::message_generator> tools_add_watermark::get(session_data_ptr in_handle) {
  bool l_preview{};
  for (auto&& [l_key, l_value, l_has_value] : in_handle->url_.params()) {
    if (l_key == "preview") {
      l_preview = true;
      break;
    }
  }

  nlohmann::json l_json{};
  if (!FSys::exists(core_set::get_set().get_cache_root() / tools_add_watermark_arg_t::g_config_name))
    l_json = tools_add_watermark_arg_t{};
  else
    l_json = nlohmann::json::parse(
        FSys::ifstream{core_set::get_set().get_cache_root() / tools_add_watermark_arg_t::g_config_name}
    );

  if (l_preview) {
    auto l_args = l_json.get<tools_add_watermark_arg_t>();
    cv::Ptr<cv::freetype::FreeType2> const l_ft2{cv::freetype::createFreeType2()};
    l_ft2->loadFontData(std::string{doodle_config::font_default}, 0);

    constexpr std::int32_t l_thickness = -1;
    std::int32_t l_baseline            = 0;
    auto l_text_size   = l_ft2->getTextSize(l_args.watermark_text_, l_args.watermark_height_, l_thickness, &l_baseline);
    cv::Scalar l_color = cv::Scalar::all(255);
    if (!l_args.watermark_color_.empty() && l_args.watermark_color_.front() == '#' &&
        l_args.watermark_color_.length() == 7) {
      l_color = cv::Scalar{
          boost::numeric_cast<std::double_t>(std::stoi(l_args.watermark_color_.substr(5, 2), nullptr, 16)),
          boost::numeric_cast<std::double_t>(std::stoi(l_args.watermark_color_.substr(3, 2), nullptr, 16)),
          boost::numeric_cast<std::double_t>(std::stoi(l_args.watermark_color_.substr(1, 2), nullptr, 16)),
      };
    }

    /// 创建一张黑色图片
    auto l_image = cv::Mat::zeros(l_args.image_size_.second, l_args.image_size_.first, CV_8UC3);
    auto l_out   = add_watermark_to_image(l_image, l_args, l_ft2, l_text_size, l_color, l_thickness);
    std::vector<uchar> l_buffer{};
    cv::imencode(".png", l_out, l_buffer, {cv::IMWRITE_PNG_BILEVEL, 0});
    co_return in_handle->make_msg(std::move(l_buffer), "image/png");
  }

  co_return in_handle->make_msg(l_json);
}

boost::asio::awaitable<boost::beast::http::message_generator> tools_add_watermark::post(session_data_ptr in_handle) {
  auto l_args = in_handle->get_json().get<tools_add_watermark_arg_t>();
  FSys::ofstream{core_set::get_set().get_cache_root() / tools_add_watermark_arg_t::g_config_name}
      << in_handle->get_json().dump(2);

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
  auto l_uuid = core_set::get_set().get_uuid();
  boost::asio::post(g_io_context(), [l_args, l_ft2, l_text_size, l_color, l_thickness, l_uuid]() {
    for (auto&& l_image_path : l_args.image_paths_) {
      if (!exists(l_image_path))
        throw_exception(
            http_request_error{
                boost::beast::http::status::bad_request,
                fmt::format("图片文件不存在: {}", l_image_path.generic_string())
            }
        );
      auto l_image = cv::imread(l_image_path.generic_string(), cv::IMREAD_UNCHANGED);
      if (l_image.empty()) continue;
      auto l_out = add_watermark_to_image(l_image, l_args, l_ft2, l_text_size, l_color, l_thickness);
      cv::imwrite((l_args.out_path_ / l_image_path.filename().replace_extension(".png")).generic_string(), l_out);
      socket_io::broadcast(
          "tools:add_watermark:progress",
          nlohmann::json{
              {"id", l_uuid},
              {"image_path", l_image_path.generic_string()},
              {"out_path", (l_args.out_path_ / l_image_path.filename().replace_extension(".png")).generic_string()}
          },
          "/events"
      );
    }
  });
  co_return in_handle->make_msg(nlohmann::json{{"id", l_uuid}});
}

boost::asio::awaitable<boost::beast::http::message_generator> tools_add_watermark::put(session_data_ptr in_handle) {
  auto l_args = in_handle->get_json().get<tools_add_watermark_arg_t>();
  FSys::ofstream{core_set::get_set().get_cache_root() / tools_add_watermark_arg_t::g_config_name}
      << in_handle->get_json().dump(2);

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

  /// 创建一张黑色图片
  auto l_image = cv::Mat::zeros(l_args.image_size_.second, l_args.image_size_.first, CV_8UC3);
  auto l_out   = add_watermark_to_image(l_image, l_args, l_ft2, l_text_size, l_color, l_thickness);
  std::vector<uchar> l_buffer{};
  cv::imencode(".png", l_image, l_buffer, {cv::IMWRITE_PNG_BILEVEL, 0});
  co_return in_handle->make_msg(std::move(l_buffer), "image/png");
}

}  // namespace doodle::http::local