//
// Created by TD on 24-10-18.
//

#include "thumbnail.h"

#include "doodle_lib/core/http/http_function.h"
#include <doodle_lib/http_method/kitsu/kitsu.h>

#include <opencv2/opencv.hpp>
namespace doodle::http::kitsu {

namespace {

boost::asio::awaitable<boost::beast::http::message_generator> thumbnail_post(session_data_ptr in_handle) {
  std::string l_name{};
  switch (in_handle->content_type_) {
    // case detail::content_type::image_gif:
    case detail::content_type::image_jpeg:
    case detail::content_type::image_png:
    default:
      co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "错误的请求类型");
      break;
  }
  FSys::path l_path = g_ctx().get<kitsu_ctx_t>().root_;
  try {
    l_name = in_handle->capture_->get("id");
  } catch (...) {
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::not_found, boost::current_exception_diagnostic_information()
    );
  }
  try {
    auto& l_data = std::get<std::string>(in_handle->body_);
    cv::Mat l_image =
        cv::imdecode(cv::InputArray{l_data.data(), boost::numeric_cast<int>(l_data.size())}, cv::IMREAD_UNCHANGED);
    if (l_image.empty())
      co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "图片解码失败");
    if (!FSys::exists(l_path / "thumbnails")) FSys::create_directories(l_path / "thumbnails");
    if (!FSys::exists(l_path / "previews")) FSys::create_directories(l_path / "previews");

    cv::imwrite((l_path / "thumbnails" / (l_name + ".png")).generic_string(), l_image);
    if (l_image.cols > 192 || l_image.rows > 108) {
      auto l_resize = std::min(192.0 / l_image.cols, 108.0 / l_image.rows);
      cv::resize(l_image, l_image, cv::Size{}, l_resize, l_resize);
    }
    cv::imwrite((l_path / "previews" / (l_name + ".png")).generic_string(), l_image);
  } catch (...) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "图片解码失败");
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
  boost::beast::http::response<boost::beast::http::file_body> l_res{
      boost::beast::http::status::ok, in_handle->version_
  };
  l_res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  l_res.set(boost::beast::http::field::content_type, kitsu::mime_type(l_ext));
  l_res.body().open(l_path.generic_string().c_str(), boost::beast::file_mode::scan, l_code);
  if (l_code)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::service_unavailable, l_code.message());
  l_res.keep_alive(in_handle->keep_alive_);
  l_res.prepare_payload();
  co_return std::move(l_res);
}
}  // namespace

void thumbnail_reg(http_route& route) {
  auto l_thumbnails_root = std::make_shared<FSys::path>(g_ctx().get<kitsu_ctx_t>().root_ / "thumbnails");
  auto l_previews_root   = std::make_shared<FSys::path>(g_ctx().get<kitsu_ctx_t>().root_ / "previews");
  route
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::post, "api/doodle/pictures/previews/{id}", thumbnail_post
          )
      )
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::get, "api/pictures/thumbnails/preview-files/{id}",
              std::bind_front(thumbnail_get, l_thumbnails_root)
          )
      )
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::get, "/api/pictures/previews/preview-files/{id}",
              std::bind_front(thumbnail_get, l_previews_root)
          )
      )

      ;
}
}  // namespace doodle::http::kitsu