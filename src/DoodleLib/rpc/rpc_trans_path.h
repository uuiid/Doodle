//
// Created by TD on 2021/7/14.
//

#pragma once
#include <DoodleLib/doodle_lib_fwd.h>

namespace doodle {
class DOODLELIB_API rpc_trans_path : public details::no_copy {
 public:
  rpc_trans_path() = default;
  explicit rpc_trans_path(FSys::path in_local, FSys::path in_server)
      : local_path(std::move(in_local)),
        server_path(std::move(in_server)),
        backup_path(FSys::add_time_stamp(in_server)){};
  explicit rpc_trans_path(FSys::path in_local, FSys::path in_server, FSys::path in_backup_path)
      : local_path(std::move(in_local)),
        server_path(std::move(in_server)),
        backup_path(std::move(in_backup_path)){};

  FSys::path local_path;
  FSys::path server_path;
  FSys::path backup_path;
};
using rpc_trans_path_ptr = std::unique_ptr<rpc_trans_path>;
using rpc_trans_path_ptr_list = std::vector<rpc_trans_path_ptr>;
}  // namespace doodle
