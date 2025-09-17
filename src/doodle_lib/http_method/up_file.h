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

class up_file_asset_base : public http_jwt_fun {
 protected:
  struct task_info_t {
    nlohmann::json task_data_{};
    uuid task_type_id_{};
    uuid entity_type_id_{};

    std::int32_t gui_dang_{};
    std::int32_t kai_shi_ji_shu_{};
    std::string bian_hao_{};
    std::string pin_yin_ming_cheng_{};
    std::string version_{};

    FSys::path root_path_{};
    FSys::path file_path_{};
    FSys::path asset_root_path_{};
  };

  virtual FSys::path gen_file_path() = 0;
  virtual void move_file(session_data_ptr in_handle);
  task_info_t task_info_{};
  DOODLE_HTTP_FUN_OVERRIDE(post)
  DOODLE_HTTP_FUN_OVERRIDE(get)
 public:
  uuid id_{};
};

// "api/doodle/data/asset/{task_id}/file/maya"
DOODLE_HTTP_FUN_C(doodle_data_asset_file_maya, up_file_asset_base)
FSys::path gen_file_path() override;
DOODLE_HTTP_FUN_END()
// "api/doodle/data/asset/{task_id}/file/ue"
DOODLE_HTTP_FUN_C(doodle_data_asset_file_ue, up_file_asset_base)
FSys::path gen_file_path() override;
DOODLE_HTTP_FUN_END()
// "api/doodle/data/asset/{task_id}/file/image"
DOODLE_HTTP_FUN_C(doodle_data_asset_file_image, up_file_asset_base)
FSys::path gen_file_path() override;
DOODLE_HTTP_FUN_END()
}  // namespace doodle::http