//
// Created by TD on 2026-11-6.
//

#include "face_capture.h"

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/face_capture/capture_session.h>
#include <doodle_lib/core/face_capture/frame_align.h>
#include <doodle_lib/core/file_sys.h>
#include <doodle_lib/core/http/http_body.h>
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/uuid/uuid_io.hpp>

#include <algorithm>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace doodle::http::local {

namespace {

std::string uuid_string(boost::uuids::uuid in_id) { return fmt::format("{}", in_id); }

nlohmann::json capture_status_json(const face_capture::capture_status& in_status) {
  return nlohmann::json{
      {"state", face_capture::to_string(in_status.state_)},
      {"width", in_status.width_},
      {"height", in_status.height_},
      {"fps", in_status.fps_},
      {"frames", in_status.frames_},
      {"duration_ms", in_status.duration_ms_},
      {"source_fps", in_status.source_fps_},
      {"truncated", in_status.truncated_}
  };
}

std::string state_of(const std::shared_ptr<face_capture::capture_session>& in_session) {
  if (!in_session) return "idle";
  return face_capture::to_string(in_session->state());
}

FSys::path session_dir_of(const boost::uuids::uuid& in_session_id) {
  return face_capture::capture_session::default_root() / uuid_string(in_session_id);
}

nlohmann::json artifacts_of(const FSys::path& in_dir) {
  auto l_exists = [&in_dir](std::string_view in_name) {
    return FSys::exists(in_dir / std::string{in_name});
  };
  std::size_t l_frames = 0;
  if (l_exists("frames")) {
    for (const auto& l_entry : FSys::directory_iterator{in_dir / "frames"})
      if (l_entry.path().extension() == ".png") ++l_frames;
  }
  return nlohmann::json{
      {"take_mp4", l_exists("take.mp4")},
      {"align_json", l_exists("align.json")},
      {"frames", l_frames},
      {"curves", FSys::exists(in_dir / "curves" / "face_curves.dhfc")}
  };
}

nlohmann::json recording_json(const FSys::path& in_dir) {
  auto l_json = nlohmann::json::object();
  try {
    const auto l_record = face_capture::read_align(in_dir);
    l_json["session_id"]   = uuid_string(l_record.session_id_);
    l_json["width"]        = l_record.width_;
    l_json["height"]       = l_record.height_;
    l_json["fps"]          = l_record.fps_;
    l_json["source_fps"]   = l_record.source_fps_;
    l_json["started_at"]   = l_record.started_at_;
    l_json["frames"]       = l_record.raw_frames_;
    l_json["duration_ms"]  = l_record.duration_ms_;
    l_json["truncated"]    = l_record.truncated_;
    l_json["gaps"]         = nlohmann::json(l_record.gaps_);
  } catch (...) { SPDLOG_WARN("读取 align.json 失败: {}", in_dir.string()); }
  return l_json;
}

}  // namespace

struct face_capture_session_holder::impl {
  mutable std::mutex mutex_{};
  std::shared_ptr<face_capture::capture_session> session_{};
};

face_capture_session_holder::face_capture_session_holder() : impl_ptr_(std::make_shared<impl>()) {}

std::shared_ptr<face_capture::capture_session> face_capture_session_holder::session() const {
  std::scoped_lock l_lock{impl_ptr_->mutex_};
  return impl_ptr_->session_;
}

void face_capture_session_holder::reset(std::shared_ptr<face_capture::capture_session> in_session) {
  std::scoped_lock l_lock{impl_ptr_->mutex_};
  if (in_session && impl_ptr_->session_ && impl_ptr_->session_ != in_session) return;
  impl_ptr_->session_ = std::move(in_session);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(face_capture_recordings, get) {
  auto l_root = face_capture::capture_session::default_root();
  auto l_list = nlohmann::json::array();
  if (FSys::exists(l_root)) {
    for (const auto& l_entry : FSys::directory_iterator{l_root}) {
      if (!l_entry.is_directory()) continue;
      if (!FSys::exists(l_entry.path() / "take.mp4") && !FSys::exists(l_entry.path() / "align.json")) continue;
      auto l_json   = recording_json(l_entry.path());
      l_json["session_id"] = l_entry.path().filename().string();
      l_list.emplace_back(std::move(l_json));
    }
  }
  co_return std::make_unique<http_body_json_t>(
      nlohmann::json{{"count", l_list.size()}, {"data", std::move(l_list)}}
  );
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(face_capture_recordings, post) {
  auto l_session = holder_->session();
  DOODLE_CHICK_HTTP(l_session != nullptr, conflict, "当前没有可用的预览连接");

  auto l_status = co_await l_session->start_record();
  auto l_json   = capture_status_json(l_status);
  l_json["session_id"] = uuid_string(l_session->session_id());
  co_return std::make_unique<http_body_json_t>(std::move(l_json));
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(face_capture_recording_stop, post) {
  auto l_session = holder_->session();
  DOODLE_CHICK_HTTP(l_session != nullptr, not_found, "没有活动会话");
  DOODLE_CHICK_HTTP(
      l_session->session_id() == session_id_, not_found, "没有 session_id = {} 的活动录制", uuid_string(session_id_)
  );

  auto l_status = co_await l_session->stop();

  co_return std::make_unique<http_body_json_t>(capture_status_json(l_status));
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(face_capture_recording, get) {
  auto l_dir = session_dir_of(session_id_);
  DOODLE_CHICK_HTTP(FSys::exists(l_dir / "take.mp4") || FSys::exists(l_dir / "align.json"), not_found, "会话不存在");

  auto l_json = recording_json(l_dir);

  auto l_session = holder_->session();
  if (l_session && l_session->session_id() == session_id_ && !l_json.contains("frames")) {
    l_json["frames"] = l_session->frames();
  }
  l_json["session_id"] = uuid_string(session_id_);
  l_json["state"]      = (l_session && l_session->session_id() == session_id_) ? state_of(l_session) : "stopped";
  l_json["artifacts"]  = artifacts_of(l_dir);

  co_return std::make_unique<http_body_json_t>(std::move(l_json));
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(face_capture_recording, delete_) {
  auto l_session = holder_->session();
  DOODLE_CHICK_HTTP(
      !(l_session && l_session->session_id() == session_id_ &&
        l_session->state() == face_capture::capture_state::recording),
      conflict, "会话正在录制中"
  );

  auto l_dir = session_dir_of(session_id_);
  DOODLE_CHICK_HTTP(FSys::exists(l_dir), not_found, "会话不存在");
  FSys::remove_all(l_dir);

  co_return std::make_unique<http_body_json_t>(nlohmann::json{{"ok", true}});
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(face_capture_recording_video, get) {
  auto l_path = session_dir_of(session_id_) / "take.mp4";
  DOODLE_CHICK_HTTP(FSys::exists(l_path), not_found, "take.mp4 不存在");
  co_return std::make_unique<http_body_file_t>(l_path, "video/mp4");
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(face_capture_recording_curves, get) {
  auto l_path = session_dir_of(session_id_) / "curves" / "face_curves.dhfc";
  DOODLE_CHICK_HTTP(FSys::exists(l_path), not_found, "face_curves.dhfc 不存在");
  auto l_body = std::make_unique<http_body_file_t>(l_path, "application/octet-stream");
  l_body->set_attachment(fmt::format("{}.dhfc", uuid_string(session_id_)));
  co_return l_body;
}

}  // namespace doodle::http::local
