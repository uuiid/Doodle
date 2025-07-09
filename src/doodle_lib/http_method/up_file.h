//
// Created by TD on 25-1-11.
//

#pragma once
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_method/http_jwt_fun.h>

#include "up_file.h"
namespace doodle {
struct entity_asset_extend;
struct asset_type;
struct task_type;
}  // namespace doodle
namespace doodle::http {

class up_file_asset_base : public http_jwt_fun_template<capture_id_t> {
 protected:
  struct task_info_t {
    nlohmann::json task_data_{};

    std::string entity_type_{};

    std::int32_t gui_dang_{};
    std::int32_t kai_shi_ji_shu_{};
    std::string bian_hao_{};
    std::string pin_yin_ming_cheng_{};
    std::string version_{};

    FSys::path root_path_{};
    FSys::path file_path_{};
  };

  std::shared_ptr<task_info_t> check_data(const entity_asset_extend& in_entity_asset_extend);
  virtual FSys::path gen_file_path(const std::shared_ptr<task_info_t>& in_data)                   = 0;
  virtual void move_file(session_data_ptr in_handle, const std::shared_ptr<task_info_t>& in_data) = 0;

 public:
  using http_jwt_fun_template<capture_id_t>::http_jwt_fun_template;
  boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
      session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
  ) override;
};
class up_file_asset : public up_file_asset_base {
 protected:
  void move_file(session_data_ptr in_handle, const std::shared_ptr<task_info_t>& in_data) override;

 public:
  using up_file_asset_base::up_file_asset_base;
};

// "api/doodle/data/asset/{task_id}/file/maya"
DOODLE_HTTP_FUN(
    up_file_asset_maya, post,
    ucom_t{} / "api" / "doodle" / "data" / "asset" / make_cap(g_uuid_regex, &capture_id_t::id_) / "file" / "maya",
    up_file_asset
)
FSys::path gen_file_path(const std::shared_ptr<task_info_t>& in_data) override;
DOODLE_HTTP_FUN_END()
// "api/doodle/data/asset/{task_id}/file/ue"
DOODLE_HTTP_FUN(
    up_file_asset_ue, post,
    ucom_t{} / "api" / "doodle" / "data" / "asset" / make_cap(g_uuid_regex, &capture_id_t::id_) / "file" / "ue",
    up_file_asset
)
FSys::path gen_file_path(const std::shared_ptr<task_info_t>& in_data) override;
void move_file(session_data_ptr in_handle, const std::shared_ptr<task_info_t>& in_data) override;
DOODLE_HTTP_FUN_END()
// "api/doodle/data/asset/{task_id}/file/image"
DOODLE_HTTP_FUN(
    up_file_asset_image, post,
    ucom_t{} / "api" / "doodle" / "data" / "asset" / make_cap(g_uuid_regex, &capture_id_t::id_) / "file" / "image",
    up_file_asset
)
FSys::path gen_file_path(const std::shared_ptr<task_info_t>& in_data) override;
DOODLE_HTTP_FUN_END()
}  // namespace doodle::http