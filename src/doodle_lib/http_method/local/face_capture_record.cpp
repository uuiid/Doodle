//
// Created by TD on 2026-11-6.
//

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/face_capture/capture_session.h>
#include <doodle_lib/core/file_sys.h>
#include <doodle_lib/core/http/http_body.h>
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/uuid/uuid_io.hpp>

#include "face_capture.h"
#include <algorithm>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace doodle::http::local {

namespace {

FSys::path session_dir_of(const boost::uuids::uuid& in_session_id) {
  return face_capture::capture_session::default_root() / fmt::to_string(in_session_id);
}

bool is_current_session(
    const std::shared_ptr<face_capture::capture_session>& in_session, const boost::uuids::uuid& in_id
) {
  if (!in_session) return false;
  return in_session->session_dir().filename().string() == fmt::to_string(in_id);
}

bool has_video(const FSys::path& in_dir) {
  return FSys::exists(in_dir / "take.mp4") || FSys::exists(in_dir / "take.webm");
}

nlohmann::json artifacts_of(const FSys::path& in_dir) {
  auto l_exists        = [&in_dir](std::string_view in_name) { return FSys::exists(in_dir / std::string{in_name}); };
  std::size_t l_frames = 0;
  if (l_exists("frames")) {
    for (const auto& l_entry : FSys::directory_iterator{in_dir / "frames"})
      if (l_entry.path().extension() == ".png") ++l_frames;
  }
  return nlohmann::json{
      {"take_mp4", l_exists("take.mp4")},
      {"take_webm", l_exists("take.webm")},
      {"frames", l_frames},
      {"curves", FSys::exists(in_dir / "curves" / "face_curves.dhfc")}
  };
}

nlohmann::json recording_json(const FSys::path& in_dir) {
  auto l_json          = nlohmann::json::object();
  l_json["session_id"] = in_dir.filename().string();
  l_json["path"]       = in_dir;
  return l_json;
}

}  // namespace

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(face_capture_recordings, get) {
  auto l_root = face_capture::capture_session::default_root();
  auto l_list = nlohmann::json::array();
  if (FSys::exists(l_root)) {
    for (const auto& l_entry : FSys::directory_iterator{l_root}) {
      if (!l_entry.is_directory()) continue;
      if (!has_video(l_entry.path())) continue;
      auto l_json          = recording_json(l_entry.path());
      l_json["session_id"] = l_entry.path().filename().string();
      l_list.emplace_back(std::move(l_json));
    }
  }
  co_return std::make_unique<http_body_json_t>(nlohmann::json{{"count", l_list.size()}, {"data", std::move(l_list)}});
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(face_capture_recordings, post) {
  auto l_session = holder_->session();
  DOODLE_CHICK_HTTP(l_session != nullptr, conflict, "当前没有可用的预览连接");

  auto l_status         = co_await l_session->start_record();
  nlohmann::json l_json = l_status;
  l_json["session_id"]  = l_session->session_dir().filename().string();
  co_return std::make_unique<http_body_json_t>(std::move(l_json));
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(face_capture_recording_upload, post) {
  auto l_file = in_handle->get_file();
  DOODLE_CHICK_HTTP(!l_file.empty() && FSys::exists(l_file), bad_request, "请求体中没有视频文件");

  auto l_name = FSys::path{std::string{in_handle->req_header_[boost::beast::http::field::content_disposition]}};
  auto l_ext  = l_name.extension().string();
  DOODLE_CHICK_HTTP(
      l_ext == ".webm" || l_ext == ".mp4", bad_request, "不支持的视频扩展名 {}，只接受 .webm 或 .mp4", l_ext
  );

  auto l_session_id = core_set::get_set().get_uuid();
  auto l_dir        = face_capture::capture_session::default_root() / fmt::format("{}", l_session_id);
  FSys::create_directories(l_dir);

  auto l_target = l_dir / fmt::format("take{}", l_ext);
  FSys::rename(l_file, l_target);

  co_return std::make_unique<http_body_json_t>(
      nlohmann::json{{"session_id", fmt::to_string(l_session_id)}, {"video", l_target.filename().string()}}
  );
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(face_capture_recording_stop, post) {
  auto l_session = holder_->session();
  DOODLE_CHICK_HTTP(l_session != nullptr, not_found, "没有活动会话");

  co_return std::make_unique<http_body_json_t>(nlohmann::json{} = (co_await l_session->stop()));
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(face_capture_recording, get) {
  auto l_dir = session_dir_of(session_id_);
  DOODLE_CHICK_HTTP(has_video(l_dir) || FSys::exists(l_dir / "curves"), not_found, "会话不存在");

  auto l_json    = recording_json(l_dir);

  auto l_session = holder_->session();
  if (is_current_session(l_session, session_id_) && !l_json.contains("frames")) {
    l_json["frames"] = l_session->frames();
  }
  l_json["session_id"] = session_id_;
  l_json["state"] =
      is_current_session(l_session, session_id_) ? l_session->state() : face_capture::capture_state::stopped;
  l_json["artifacts"] = artifacts_of(l_dir);

  co_return std::make_unique<http_body_json_t>(std::move(l_json));
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(face_capture_recording, delete_) {
  auto l_session = holder_->session();
  DOODLE_CHICK_HTTP(
      !(is_current_session(l_session, session_id_) && l_session->state() == face_capture::capture_state::recording),
      conflict, "会话正在录制中"
  );

  auto l_dir = session_dir_of(session_id_);
  DOODLE_CHICK_HTTP(FSys::exists(l_dir), not_found, "会话不存在");
  FSys::remove_all(l_dir);

  co_return std::make_unique<http_body_json_t>(nlohmann::json{{"ok", true}});
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(face_capture_recording_video, get) {
  auto l_dir  = session_dir_of(session_id_);
  auto l_path = l_dir / "take.mp4";
  auto l_mime = std::string{"video/mp4"};
  if (!FSys::exists(l_path)) {
    l_path = l_dir / "take.webm";
    l_mime = "video/webm";
  }
  DOODLE_CHICK_HTTP(FSys::exists(l_path), not_found, "录制视频不存在");
  co_return std::make_unique<http_body_file_t>(l_path, l_mime);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(face_capture_recording_video, head) { return get(in_handle); }

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(face_capture_recording_curves, get) {
  auto l_path = session_dir_of(session_id_) / "curves" / "face_curves.dhfc";
  DOODLE_CHICK_HTTP(FSys::exists(l_path), not_found, "face_curves.dhfc 不存在");
  auto l_body = std::make_unique<http_body_file_t>(l_path, "application/octet-stream");
  l_body->set_attachment(fmt::format("{}.dhfc", session_id_));
  co_return l_body;
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(face_capture_recording_curves, head) { return get(in_handle); }

}  // namespace doodle::http::local
