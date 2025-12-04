//
// Created by TD on 25-1-11.
//

#pragma once
#include "doodle_core/metadata/entity.h"
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_method/http_jwt_fun.h>

#include <string>

namespace doodle {
struct entity_asset_extend;
struct asset_type;
struct task_type;
}  // namespace doodle
namespace doodle::http {

class up_file_base : public http_jwt_fun {
 protected:
  virtual void query_task_info(session_data_ptr in_handle) = 0;
  virtual FSys::path gen_file_path()                       = 0;
  virtual void move_file(session_data_ptr in_handle);
  FSys::path file_path_{};
  FSys::path root_path_{};

 public:
  uuid id_{};
  DOODLE_HTTP_FUN_OVERRIDE(post)
  DOODLE_HTTP_FUN_OVERRIDE(get)
  DOODLE_HTTP_FUN_OVERRIDE(delete_)
};

class up_file_asset_base : public up_file_base {
 protected:
  uuid task_type_id_{};
  uuid entity_type_id_{};

  std::int32_t gui_dang_{};
  std::int32_t kai_shi_ji_shu_{};
  std::string bian_hao_{};
  std::string pin_yin_ming_cheng_{};
  std::string version_{};

  FSys::path asset_root_path_{};

  virtual void query_task_info(session_data_ptr in_handle) override;

 public:
  uuid id_{};
};

class up_file_shots_base : public up_file_base {
 protected:
  std::string episode_name_{};
  std::string shot_name_{};
  std::string project_code_{};
  entity episode_;
  entity shot_;
  uuid task_type_id_{};
  virtual void query_task_info(session_data_ptr in_handle) override;

 protected:
};

// /api/doodle/data/assets/{task_id}/file/maya
DOODLE_HTTP_FUN_C(doodle_data_asset_file_maya, up_file_asset_base)
FSys::path gen_file_path() override;
DOODLE_HTTP_FUN_END()
// /api/doodle/data/assets/{task_id}/file/ue
DOODLE_HTTP_FUN_C(doodle_data_asset_file_ue, up_file_asset_base)
FSys::path gen_file_path() override;
DOODLE_HTTP_FUN_END()
// /api/doodle/data/assets/{task_id}/file/image
DOODLE_HTTP_FUN_C(doodle_data_asset_file_image, up_file_asset_base)
FSys::path gen_file_path() override;
DOODLE_HTTP_FUN_END()
// /api/doodle/data/shots/{task_id}/file/maya
DOODLE_HTTP_FUN_C(doodle_data_shots_file_maya, up_file_shots_base)
FSys::path gen_file_path() override;
DOODLE_HTTP_FUN_END()
// /api/doodle/data/shots/{task_id}/file/output
DOODLE_HTTP_FUN_C(doodle_data_shots_file_output, up_file_shots_base)
FSys::path gen_file_path() override;
DOODLE_HTTP_FUN_END()
// /api/doodle/data/shots/{task_id}/file/other
DOODLE_HTTP_FUN_C(doodle_data_shots_file_other, up_file_shots_base)
FSys::path gen_file_path() override;
DOODLE_HTTP_FUN_END()
// /api/doodle/data/shots/{task_id}/file/ue
DOODLE_HTTP_FUN_C(doodle_data_shots_file_ue, up_file_shots_base)
FSys::path gen_file_path() override;
DOODLE_HTTP_FUN_END()
}  // namespace doodle::http