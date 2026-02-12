//
// Created by TD on 24-10-18.
//

#include "doodle_core/metadata/ai_image_metadata.h"
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/zlib_deflate_file_body.h>
#include <doodle_lib/http_method/kitsu.h>

#include <doodle_core/core/global_function.h>

#include "model_library.h"
#include <memory>
#include <opencv2/opencv.hpp>
#include <spdlog/spdlog.h>
#include <utility>

namespace doodle::http::model_library {

std::pair<std::size_t, std::size_t> pictures_base::create_thumbnail_image(
    const std::string& in_data, const FSys::path& in_path, FSys::path in_name
) {
  auto l_thumbnails_path = in_path / "thumbnails" / in_name.replace_extension(".png");
  auto l_preview_path    = in_path / "previews" / in_name.replace_extension(".png");

  if (auto l_p = l_thumbnails_path.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
  if (auto l_p = l_preview_path.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
  cv::Mat l_image = cv::imdecode(
      cv::InputArray{reinterpret_cast<const uchar*>(in_data.data()), boost::numeric_cast<int>(in_data.size())},
      cv::IMREAD_COLOR
  );
  if (l_image.empty()) throw_exception(doodle_error{"图片解码失败"});

  std::pair<std::size_t, std::size_t> l_size = {l_image.cols, l_image.rows};

  cv::imwrite(l_preview_path.generic_string(), l_image);
  if (l_image.cols > 192 || l_image.rows > 108) {
    auto l_resize = std::min(192.0 / l_image.cols, 108.0 / l_image.rows);
    cv::resize(l_image, l_image, cv::Size{}, l_resize, l_resize);
  }
  cv::imwrite(l_thumbnails_path.generic_string(), l_image);

  return l_size;
}
std::pair<std::size_t, std::size_t> pictures_base::create_thumbnail_gif(
    const FSys::path& in_data_path, const FSys::path& in_path, FSys::path in_name
) {
  auto l_thumbnails_path = in_path / "thumbnails" / in_name.replace_extension(".png");
  auto l_preview_path    = in_path / "previews" / in_name.replace_extension(".gif");

  if (auto l_p = l_thumbnails_path.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
  if (auto l_p = l_preview_path.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);

  cv::VideoCapture l_video{};
  l_video.open(in_data_path.generic_string());
  if (!l_video.isOpened()) throw_exception(doodle_error{"gif 打开失败"});

  auto l_video_count = l_video.get(cv::CAP_PROP_FRAME_COUNT);
  cv::Mat l_image{};
  l_video.set(cv::CAP_PROP_POS_FRAMES, std::clamp(l_video_count / 2, 0.0, l_video_count - 1));
  l_video >> l_image;
  if (l_image.empty()) throw_exception(doodle_error{"图片解码失败"});

  std::pair<std::size_t, std::size_t> l_size = {l_image.cols, l_image.rows};

  if (l_image.cols > 192 || l_image.rows > 108) {
    auto l_resize = std::min(192.0 / l_image.cols, 108.0 / l_image.rows);
    cv::resize(l_image, l_image, cv::Size{}, l_resize, l_resize);
  }
  cv::imwrite(l_thumbnails_path.generic_string(), l_image);

  FSys::rename(in_data_path, l_preview_path);

  return l_size;
}
std::pair<std::size_t, std::size_t> pictures_base::create_thumbnail_mp4(
    const FSys::path& in_data_path, const FSys::path& in_path, FSys::path in_name
) {
  auto l_thumbnails_path = in_path / "thumbnails" / in_name.replace_extension(".png");
  auto l_preview_path    = in_path / "previews" / in_name.replace_extension(".mp4");
  if (auto l_p = l_thumbnails_path.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
  if (auto l_p = l_preview_path.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
  std::pair<std::size_t, std::size_t> l_size{};
  {
    cv::VideoCapture l_video{};
    l_video.open(in_data_path.generic_string());
    if (!l_video.isOpened()) throw_exception(doodle_error{"mp4 打开失败"});

    auto l_video_count = l_video.get(cv::CAP_PROP_FRAME_COUNT);
    cv::Mat l_image{};
    l_video.set(cv::CAP_PROP_POS_FRAMES, std::clamp(l_video_count / 2, 0.0, l_video_count - 1));
    l_video >> l_image;
    if (l_image.empty()) throw_exception(doodle_error{"图片解码失败"});
    l_size = {l_image.cols, l_image.rows};

    if (l_image.cols > 192 || l_image.rows > 108) {
      auto l_resize = std::min(192.0 / l_image.cols, 108.0 / l_image.rows);
      cv::resize(l_image, l_image, cv::Size{}, l_resize, l_resize);
    }
    cv::imwrite(l_thumbnails_path.generic_string(), l_image);
  }
  FSys::rename(in_data_path, l_preview_path);

  return l_size;
}

boost::asio::awaitable<boost::beast::http::message_generator> pictures_base::thumbnail_post(
    session_data_ptr in_handle, FSys::path in_path
) {
  std::string l_name{fmt::format("{}", id_)};
  FSys::path l_path = in_path;
  std::pair<std::size_t, std::size_t> l_size{};

  switch (in_handle->content_type_) {
    // case detail::content_type::image_gif:
    case detail::content_type::image_jpeg:
    case detail::content_type::image_jpg:
    case detail::content_type::image_png:
      l_size = create_thumbnail_image(std::get<std::string>(in_handle->body_), l_path, FSys::split_uuid_path(l_name));
      break;
    case detail::content_type::image_gif:
      l_size = create_thumbnail_gif(std::get<FSys::path>(in_handle->body_), l_path, FSys::split_uuid_path(l_name));
      break;
    case detail::content_type::video_mp4:
      l_size = create_thumbnail_mp4(std::get<FSys::path>(in_handle->body_), l_path, FSys::split_uuid_path(l_name));
      break;
    default:
      co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "错误的请求类型");
      break;
  }

  auto l_sql = g_ctx().get<sqlite_database>();
  if (auto l_id = l_sql.uuid_to_id<ai_image_metadata>(id_); l_id != 0) {
    auto l_entt     = std::make_shared<ai_image_metadata>(l_sql.get_by_uuid<ai_image_metadata>(id_));
    l_entt->width_  = l_size.first;
    l_entt->height_ = l_size.second;
    co_await l_sql.update(l_entt);
  }

  co_return in_handle->make_msg(fmt::format(R"({{"id":"{}"}})", id_));
}

boost::asio::awaitable<boost::beast::http::message_generator> pictures_base::thumbnail_get(
    session_data_ptr in_handle, FSys::path in_path, std::string in_extension
) {
  FSys::path l_path = in_path;

  l_path /= fmt::format("{}{}", id_, in_extension);
  if (auto l_new_path = l_path.parent_path() / FSys::split_uuid_path(l_path.filename()); FSys::exists(l_new_path))
    l_path = l_new_path;

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

boost::asio::awaitable<boost::beast::http::message_generator> pictures_instance::post(session_data_ptr in_handle) {
  person_.check_user();
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 上传缩略图/预览文件 {} ", person_.person_.email_,
      person_.person_.get_full_name(), id_
  );
  return thumbnail_post(in_handle, *root_);
}
boost::asio::awaitable<boost::beast::http::message_generator> pictures_instance::get(session_data_ptr in_handle) {
  person_.check_user();
  return thumbnail_get(in_handle, *root_ / "previews", ".png");
}
boost::asio::awaitable<boost::beast::http::message_generator> pictures_instance_mp4::get(session_data_ptr in_handle) {
  person_.check_user();
  return thumbnail_get(in_handle, *root_ / "previews", ".mp4");
}
boost::asio::awaitable<boost::beast::http::message_generator> pictures_thumbnails::get(session_data_ptr in_handle) {
  person_.check_user();
  return thumbnail_get(in_handle, *root_ / "thumbnails");
}

}  // namespace doodle::http::model_library