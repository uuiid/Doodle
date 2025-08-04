//
// Created by TD on 25-5-13.
//
#include "local.h"
#include <cache.hpp>
#include <cache_policy.hpp>
#include <lru_cache_policy.hpp>
#include <opencv2/opencv.hpp>
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
  init_ctx();
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
  co_return in_handle->make_msg(std::move(l_buffer), "image/png", boost::beast::http::status::ok);
}
void video_thumbnail::init_ctx() {
  static std::once_flag l_flag{};
  std::call_once(l_flag, []() { g_ctx().emplace<video_thumbnail_cache>(); });
}

boost::asio::awaitable<boost::beast::http::message_generator> video_thumbnail::get(session_data_ptr in_handle) {
  init_ctx();
  auto& l_cache = g_ctx().get<video_thumbnail_cache>();
  if (l_cache.Cached(video_thumbnail_cache_id)) {
    auto l_buffer = *l_cache.Get(video_thumbnail_cache_id);
    co_return in_handle->make_msg(std::move(l_buffer), "image/png", boost::beast::http::status::ok);
  }
  throw_exception(http_request_error{boost::beast::http::status::bad_request, "视频文件不存在"});
}

}  // namespace doodle::http::local