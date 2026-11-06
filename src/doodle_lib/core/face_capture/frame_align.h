//
// Created by TD on 2026-11-6.
//

#pragma once

#include <doodle_lib/core/face_capture/face_capture_types.h>

#include <filesystem>
#include <optional>

namespace doodle::face_capture {

FSys::path align_path(const FSys::path& in_session_dir);

void write_align(const align_record& in_record, const FSys::path& in_session_dir);

align_record read_align(const FSys::path& in_session_dir);

std::optional<std::int64_t> wall_ms_of(const align_record& in_record, std::uint64_t in_seq);

std::optional<std::uint64_t> orig_seq_of(const align_record& in_record, std::uint64_t in_seq);

std::uint64_t expect_frame_count(const align_record& in_record);

}  // namespace doodle::face_capture
