//
// Created by TD on 2026-11-6.
//

#include "face_capture.h"

#include <doodle_core/exception/exception.h>

#include <doodle_lib/core/face_capture/capture_session.h>
#include <doodle_lib/core/face_capture/face_capture_types.h>
#include <doodle_lib/core/face_capture/ws_outlet.h>
#include <doodle_lib/core/http/http_body.h>
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/doodle_lib_fwd.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

namespace doodle::http::local {

struct face_capture_selected_device::impl {
  mutable std::mutex mutex_{};
  std::int32_t index_{0};
};

face_capture_selected_device::face_capture_selected_device() : impl_ptr_(std::make_shared<impl>()) {}

std::int32_t face_capture_selected_device::index() const {
  std::scoped_lock l_lock{impl_ptr_->mutex_};
  return impl_ptr_->index_;
}

void face_capture_selected_device::select(std::int32_t in_index) {
  std::scoped_lock l_lock{impl_ptr_->mutex_};
  impl_ptr_->index_ = in_index;
}

namespace {

std::vector<face_capture::camera_info> enumerate_devices() { return face_capture::capture_session::list_devices(); }

}  // namespace

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(face_capture_devices, get) {
  auto l_selected = selected_->index();
  auto l_devices  = nlohmann::json::array();
  for (const face_capture::camera_info& l_info : enumerate_devices()) {
    auto l_json        = nlohmann::json(l_info);
    l_json["selected"] = l_info.index_ == l_selected;
    l_devices.emplace_back(std::move(l_json));
  }
  co_return std::make_unique<http_body_json_t>(
      nlohmann::json{{"selected", l_selected}, {"devices", std::move(l_devices)}}
  );
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(face_capture_devices, post) {
  auto l_json = in_handle->get_json();
  DOODLE_CHICK_HTTP(
      l_json.is_object() && l_json.contains("index") && l_json["index"].is_number_integer(), bad_request,
      "请求体需要整数 index"
  );
  auto l_index = l_json["index"].get<std::int32_t>();
  DOODLE_CHICK_HTTP(l_index >= 0, bad_request, "index 不能为负");

  auto l_devices = enumerate_devices();
  DOODLE_CHICK_HTTP(
      std::any_of(
          l_devices.begin(), l_devices.end(), [l_index](const face_capture::camera_info& in) {
            return in.index_ == l_index;
          }
      ),
      not_found, "没有 index = {} 的摄像头", l_index
  );

  selected_->select(l_index);
  co_return std::make_unique<http_body_json_t>(nlohmann::json{{"selected", l_index}});
}

bool face_capture_stream::has_websocket() const { return true; }

void face_capture_stream::websocket_init(session_data_ptr in_handle) {}

void face_capture_stream::websocket_callback(
    boost::beast::websocket::stream<tcp_stream_type> in_stream, session_data_ptr in_handle
) {
  std::make_shared<face_capture::ws_outlet>(std::move(in_stream), selected_ ? selected_->index() : 0, holder_)
      ->start();
}

}  // namespace doodle::http::local
