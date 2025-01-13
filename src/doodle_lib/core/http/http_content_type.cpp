//
// Created by TD on 25-1-13.
//

#include "http_content_type.h"
namespace doodle::http::detail {
content_type get_content_type(const std::string_view& in_content_type) {
  if (in_content_type.starts_with("application/json")) return content_type::application_json;
  if (in_content_type.starts_with("image/jpeg")) return content_type::image_jpeg;
  if (in_content_type.starts_with("image/jpg")) return content_type::image_jpg;
  if (in_content_type.starts_with("image/png")) return content_type::image_png;
  if (in_content_type.starts_with("image/gif")) return content_type::image_gif;
  if (in_content_type.starts_with("video/mp4")) return content_type::video_mp4;
  if (in_content_type.starts_with("multipart/form-data")) return content_type::multipart_form_data;
  return content_type::unknown;
}
}  // namespace doodle::http::detail