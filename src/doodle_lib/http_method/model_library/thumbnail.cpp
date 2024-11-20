//
// Created by TD on 24-10-18.
//

#include "thumbnail.h"

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/zlib_deflate_file_body.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>

#include <opencv2/opencv.hpp>
#include <tl/expected.hpp>
namespace doodle::http::kitsu {

namespace {

tl::expected<void, std::string> create_thumbnail_image(
    const std::string& in_data, const FSys::path& in_path, const std::string& in_name
) {
  try {
    cv::Mat l_image = cv::imdecode(
        cv::InputArray{reinterpret_cast<const uchar*>(in_data.data()), boost::numeric_cast<int>(in_data.size())},
        cv::IMREAD_COLOR
    );
    if (l_image.empty()) return tl::make_unexpected("图片解码失败");

    cv::imwrite((in_path / "previews" / (in_name + ".png")).generic_string(), l_image);
    if (l_image.cols > 192 || l_image.rows > 108) {
      auto l_resize = std::min(192.0 / l_image.cols, 108.0 / l_image.rows);
      cv::resize(l_image, l_image, cv::Size{}, l_resize, l_resize);
    }
    cv::imwrite((in_path / "thumbnails" / (in_name + ".png")).generic_string(), l_image);
  } catch (...) {
    return tl::make_unexpected(fmt::format("图片解码失败 {} ", boost::current_exception_diagnostic_information()));
  }
  return {};
}
tl::expected<void, std::string> create_thumbnail_gif(
    const FSys::path& in_data_path, const FSys::path& in_path, const std::string& in_name
) {
  try {
    {
      cv::VideoCapture l_video{};
      l_video.open(in_data_path.generic_string());
      if (!l_video.isOpened()) return tl::make_unexpected("gif 打开失败");

      auto l_video_count = l_video.get(cv::CAP_PROP_FRAME_COUNT);
      cv::Mat l_image{};
      l_video.set(cv::CAP_PROP_POS_FRAMES, std::clamp(l_video_count / 2, 0.0, l_video_count - 1));
      l_video >> l_image;
      if (l_image.empty()) return tl::make_unexpected("图片解码失败");

      if (l_image.cols > 192 || l_image.rows > 108) {
        auto l_resize = std::min(192.0 / l_image.cols, 108.0 / l_image.rows);
        cv::resize(l_image, l_image, cv::Size{}, l_resize, l_resize);
      }
      cv::imwrite((in_path / "thumbnails" / (in_name + ".png")).generic_string(), l_image);
    }
    FSys::copy_file(in_data_path, in_path / "previews" / (in_name + ".gif"));
    try {
      FSys::remove(in_data_path);
    } catch (...) {
      default_logger_raw()->error("删除临时文件失败 {}", boost::current_exception_diagnostic_information());
    }
  } catch (...) {
    return tl::make_unexpected(fmt::format("图片解码失败 {} ", boost::current_exception_diagnostic_information()));
  }
  return {};
}

boost::asio::awaitable<boost::beast::http::message_generator> thumbnail_post(session_data_ptr in_handle) {
  std::string l_name{};
  FSys::path l_path = g_ctx().get<kitsu_ctx_t>().root_;
  try {
    l_name = in_handle->capture_->get("id");
  } catch (...) {
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::not_found, boost::current_exception_diagnostic_information()
    );
  }
  switch (in_handle->content_type_) {
    // case detail::content_type::image_gif:
    case detail::content_type::image_jpeg:
    case detail::content_type::image_jpg:
    case detail::content_type::image_png:
      create_thumbnail_image(std::get<std::string>(in_handle->body_), l_path, l_name);
      break;
    case detail::content_type::image_gif:
      create_thumbnail_gif(std::get<FSys::path>(in_handle->body_), l_path, l_name);
      break;
    default:
      co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "错误的请求类型");
      break;
  }

  co_return in_handle->make_msg(fmt::format(R"({{"id":"{}"}})", in_handle->capture_->get("id")));
}
boost::asio::awaitable<boost::beast::http::message_generator> thumbnail_get(
    std::shared_ptr<FSys::path> in_root, session_data_ptr in_handle
) {
  FSys::path l_path = *in_root;
  try {
    l_path /= in_handle->capture_->get("id");
    if (!FSys::exists(l_path)) {
      co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "文件不存在");
    }
  } catch (...) {
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::not_found, boost::current_exception_diagnostic_information()
    );
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
}  // namespace

void thumbnail_reg(http_route& route) {
  auto l_thumbnails_root = std::make_shared<FSys::path>(g_ctx().get<kitsu_ctx_t>().root_ / "thumbnails");
  auto l_previews_root   = std::make_shared<FSys::path>(g_ctx().get<kitsu_ctx_t>().root_ / "previews");
  if (!FSys::exists(*l_thumbnails_root)) FSys::create_directories(*l_thumbnails_root);
  if (!FSys::exists(*l_previews_root)) FSys::create_directories(*l_previews_root);
  if (auto l_p = g_ctx().get<kitsu_ctx_t>().root_ / "tmp"; !FSys::exists(l_p)) FSys::create_directories(l_p);
  route.reg(std::make_shared<http_function>(boost::beast::http::verb::post, "api/doodle/pictures/{id}", thumbnail_post))
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::get, "api/doodle/pictures/thumbnails/{id}",
              std::bind_front(thumbnail_get, l_thumbnails_root)
          )
      )
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::get, "api/doodle/pictures/{id}", std::bind_front(thumbnail_get, l_previews_root)
          )
      )

      ;
}
}  // namespace doodle::http::kitsu