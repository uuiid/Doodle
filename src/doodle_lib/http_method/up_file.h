//
// Created by TD on 25-1-11.
//

#pragma once
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
namespace doodle::http {


class up_file_asset : public http_jwt_fun {
 protected:
  nlohmann::json task_data_{};

  std::string entity_type_{};

  std::int32_t gui_dang_{};
  std::int32_t kai_shi_ji_shu_{};
  std::string bian_hao_{};
  std::string pin_yin_ming_cheng_{};
  std::string version_{};

  virtual void check_data(const nlohmann::json& in_data);
  virtual FSys::path gen_file_path() = 0;

 public:
  using http_jwt_fun::http_jwt_fun;
  boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
};

DOODLE_HTTP_FUN(up_file_asset_maya, post, "api/doodle/data/asset/{task_id}/file/maya", up_file_asset)
FSys::path gen_file_path() override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(up_file_asset_ue, post, "api/doodle/data/asset/{task_id}/file/ue", up_file_asset)
FSys::path gen_file_path() override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(up_file_asset_image, post, "api/doodle/data/asset/{task_id}/file/image", up_file_asset)
FSys::path gen_file_path() override;
DOODLE_HTTP_FUN_END()
}  // namespace doodle::http