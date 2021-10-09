//
// Created by TD on 2021/7/14.
//

#include "rpc_trans_path.h"

#include <doodle_lib/metadata/assets_path.h>
doodle::rpc_trans_path::rpc_trans_path(const assets_path_ptr& in_ptr)
    : local_path(in_ptr->get_local_path()),
      server_path(in_ptr->get_server_path()),
      backup_path(in_ptr->get_backup_path()) {
}
