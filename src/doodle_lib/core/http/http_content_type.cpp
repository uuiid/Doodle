//
// Created by TD on 25-1-13.
//

#include "http_content_type.h"

#include <string>
namespace doodle::http::detail {
content_type get_content_type(const std::string_view& in_content_type) {
  if (in_content_type.starts_with("application/json")) return content_type::application_json;
  if (in_content_type.starts_with("image/jpeg")) return content_type::image_jpeg;
  if (in_content_type.starts_with("image/jpg")) return content_type::image_jpg;
  if (in_content_type.starts_with("image/png")) return content_type::image_png;
  if (in_content_type.starts_with("image/gif")) return content_type::image_gif;
  if (in_content_type.starts_with("video/mp4")) return content_type::video_mp4;
  if (in_content_type.starts_with("multipart/form-data")) return content_type::multipart_form_data;
  if (in_content_type.starts_with("application/")) return content_type::application_nuknown;
  if (in_content_type.starts_with("text/plain")) return content_type::text_plain;
  if (in_content_type.starts_with("video/quicktime")) return content_type::video_mov;
  return content_type::unknown;
}

std::string extension_from_mime_type(detail::content_type in_mime_type) {
  switch (in_mime_type) {
    case content_type::image_jpeg:
    case content_type::image_jpg:
      return ".jpg";
    case content_type::image_png:
      return ".png";
    case content_type::image_gif:
      return ".gif";
    case content_type::video_mp4:
      return ".mp4";
    case content_type::video_mov:
      return ".mov";
    case content_type::text_plain:
      return ".txt";
    case content_type::application_nuknown:
      return ".tmp";
    default:
      return "";
  }
}

}  // namespace doodle::http::detail