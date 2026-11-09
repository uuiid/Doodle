//
// Created by TD on 2026-11-6.
//

#pragma once

#include <doodle_lib/core/face_capture/face_capture_types.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <cstdint>
#include <string>

namespace doodle::face_capture {

struct png_sequence_option {
  FSys::path video_{};
  FSys::path session_dir_{};
  std::int32_t decode_max_width_{k_default_width};
};

struct png_sequence_result {
  bool ok_{false};
  std::uint64_t frame_count_{0};
  std::int32_t width_{0};
  std::int32_t height_{0};
  FSys::path frames_dir_{};
  error_code_t error_code_{error_code_t::none};
  std::string message_{};
};

FSys::path frames_dir_of(const FSys::path& in_session_dir);

FSys::path png_tmp_dir_of(const FSys::path& in_session_dir);

png_sequence_result write_png_sequence(const png_sequence_option& in_option);

}  // namespace doodle::face_capture
