//
// Created by TD on 25-1-13.
//

#pragma once

#include <string_view>  // std::string_view
namespace doodle::http::detail {
enum class content_type {
  text_plain,
  application_json,
  application_nuknown,
  text_html,
  text_css,
  text_javascript,
  text_xml,
  image_jpeg,
  image_jpg,
  image_png,
  image_gif,
  video_mp4,
  video_mov,
  video_avi,
  audio_mp3,
  audio_wav,
  audio_ogg,
  audio_aac,
  audio_wma,
  audio_m4a,
  form_data,
  multipart_form_data,
  unknown
};

content_type get_content_type(const std::string_view& in_str);
std::string extension_from_mime_type(detail::content_type in_mime_type);

}  // namespace doodle::http::detail