//
// Created by TD on 2026-11-10.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/core/asyn_task.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_client/kitsu_client.h>

#include <boost/asio/awaitable.hpp>

#include <memory>
#include <string_view>

namespace doodle {

/**
 * @brief 面部捕捉转码: take.mp4 -> png 序列 -> UE 解算动捕文件
 */
class DOODLELIB_API face_capture_transcode_arg : public async_task {
 public:
  uuid preview_id_{};

  FSys::path ue_project_{};

  std::shared_ptr<kitsu::kitsu_client> kitsu_client_{};

  constexpr static std::string_view k_name{"face_capture_transcode"};

  friend void from_json(const nlohmann::json& in_json, face_capture_transcode_arg& out_obj);
  friend void to_json(nlohmann::json& in_json, const face_capture_transcode_arg& out_obj);
  boost::asio::awaitable<void> run() override;
};

}  // namespace doodle
