//
// Created by TD on 24-10-18.
//

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/zlib_deflate_file_body.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>

#include "model_library.h"
#include <opencv2/opencv.hpp>
namespace doodle::http::model_library {

void pictures_base::create_thumbnail_image(const std::string& in_data, const FSys::path& in_path, FSys::path in_name) {
  auto l_path = in_path / "thumbnails" / in_name.replace_extension(".png");
  if (auto l_p = l_path.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
  cv::Mat l_image = cv::imdecode(
      cv::InputArray{reinterpret_cast<const uchar*>(in_data.data()), boost::numeric_cast<int>(in_data.size())},
      cv::IMREAD_COLOR
  );
  if (l_image.empty()) return throw_exception(doodle_error{"图片解码失败"});

  cv::imwrite((in_path / "previews" / in_name.replace_extension(".png")).generic_string(), l_image);
  if (l_image.cols > 192 || l_image.rows > 108) {
    auto l_resize = std::min(192.0 / l_image.cols, 108.0 / l_image.rows);
    cv::resize(l_image, l_image, cv::Size{}, l_resize, l_resize);
  }
  cv::imwrite(l_path.generic_string(), l_image);
}
void pictures_base::create_thumbnail_gif(
    const FSys::path& in_data_path, const FSys::path& in_path, FSys::path in_name
) {
  auto l_path = in_path / "thumbnails" / in_name.replace_extension(".png");
  if (auto l_p = l_path.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
  {
    cv::VideoCapture l_video{};
    l_video.open(in_data_path.generic_string());
    if (!l_video.isOpened()) return throw_exception(doodle_error{"gif 打开失败"});

    auto l_video_count = l_video.get(cv::CAP_PROP_FRAME_COUNT);
    cv::Mat l_image{};
    l_video.set(cv::CAP_PROP_POS_FRAMES, std::clamp(l_video_count / 2, 0.0, l_video_count - 1));
    l_video >> l_image;
    if (l_image.empty()) return throw_exception(doodle_error{"图片解码失败"});

    if (l_image.cols > 192 || l_image.rows > 108) {
      auto l_resize = std::min(192.0 / l_image.cols, 108.0 / l_image.rows);
      cv::resize(l_image, l_image, cv::Size{}, l_resize, l_resize);
    }
    cv::imwrite(l_path.generic_string(), l_image);
  }
  FSys::rename(in_data_path, in_path / "previews" / in_name.replace_extension(".gif"));
}
void pictures_base::create_thumbnail_mp4(
    const FSys::path& in_data_path, const FSys::path& in_path, FSys::path in_name
) {
  cv::VideoCapture l_video{};
  l_video.open(in_data_path.generic_string());
  if (!l_video.isOpened()) throw_exception(doodle_error{"mp4 打开失败"});

  auto l_video_count = l_video.get(cv::CAP_PROP_FRAME_COUNT);
  cv::Mat l_image{};
  l_video.set(cv::CAP_PROP_POS_FRAMES, std::clamp(l_video_count / 2, 0.0, l_video_count - 1));
  l_video >> l_image;
  if (l_image.empty()) throw_exception(doodle_error{"图片解码失败"});

  if (l_image.cols > 192 || l_image.rows > 108) {
    auto l_resize = std::min(192.0 / l_image.cols, 108.0 / l_image.rows);
    cv::resize(l_image, l_image, cv::Size{}, l_resize, l_resize);
  }
  cv::imwrite((in_path / "thumbnails" / in_name.replace_extension(".png")).generic_string(), l_image);
}

boost::asio::awaitable<boost::beast::http::message_generator> pictures_base::thumbnail_post(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) {
  std::string l_name{fmt::format("{}", in_arg->id_)};
  FSys::path l_path = *root_;

  switch (in_handle->content_type_) {
    // case detail::content_type::image_gif:
    case detail::content_type::image_jpeg:
    case detail::content_type::image_jpg:
    case detail::content_type::image_png:
      create_thumbnail_image(std::get<std::string>(in_handle->body_), l_path, FSys::split_uuid_path(l_name));
      break;
    case detail::content_type::image_gif:
      create_thumbnail_gif(std::get<FSys::path>(in_handle->body_), l_path, FSys::split_uuid_path(l_name));
      break;
    default:
      co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "错误的请求类型");
      break;
  }
  co_return in_handle->make_msg(fmt::format(R"({{"id":"{}"}})", in_arg->id_));
}

boost::asio::awaitable<boost::beast::http::message_generator> pictures_base::thumbnail_get(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) {
  FSys::path l_path = *root_;

  l_path /= fmt::format("{}.png", in_arg->id_);
  if (auto l_new_path = FSys::split_uuid_path(l_path.filename()); FSys::exists(l_new_path)) l_path = l_new_path;

  if (!FSys::exists(l_path)) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "文件不存在");
  }

  auto l_ext = l_path.extension();
  boost::system::error_code l_code{};
  auto l_set_handle = [&l_ext, &in_handle](auto&& in_res) {
    in_res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    in_res.set(boost::beast::http::field::content_type, kitsu::mime_type(l_ext));
    in_res.keep_alive(in_handle->keep_alive_);
    in_res.prepare_payload();
  };

  if (in_handle->req_header_[boost::beast::http::field::accept_encoding].contains("deflate")) {
    boost::beast::http::response<http::zlib_deflate_file_body> l_res{
        boost::beast::http::status::ok, in_handle->version_
    };
    l_res.body().open(l_path, std::ios::in | std::ios::binary, l_code);
    l_res.set(boost::beast::http::field::content_encoding, "deflate");
    l_set_handle(l_res);
    if (l_code)
      co_return in_handle->make_error_code_msg(boost::beast::http::status::service_unavailable, l_code.message());
    co_return std::move(l_res);
  }
  boost::beast::http::response<boost::beast::http::file_body> l_res{
      boost::beast::http::status::ok, in_handle->version_
  };
  l_res.body().open(l_path.generic_string().c_str(), boost::beast::file_mode::scan, l_code);
  l_set_handle(l_res);
  if (l_code)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::service_unavailable, l_code.message());
  co_return std::move(l_res);
}
boost::asio::awaitable<boost::beast::http::message_generator> pictures_base::thumbnail_404(session_data_ptr in_handle) {
  co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "文件不存在");
}

boost::asio::awaitable<boost::beast::http::message_generator> pictures_base::callback_arg(
    http::session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) {
  if (verb_ == boost::beast::http::verb::post) {
    return thumbnail_post(in_handle, in_arg);
  }
  return thumbnail_get(in_handle, in_arg);
}

}  // namespace doodle::http::model_library