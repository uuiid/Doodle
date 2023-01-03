//
// Created by TD on 2022/4/29.
//

#include "rpc_client.h"

#include <doodle_core/json_rpc/exception/json_rpc_error.h>

#include <boost/asio.hpp>

#include <json_rpc/core/rpc_reply.h>
#include <json_rpc/core/server.h>
#include <json_rpc/core/session.h>
#include <utility>

namespace doodle::json_rpc {

rpc_client::rpc_client()  = default;
rpc_client::~rpc_client() = default;

}  // namespace doodle::json_rpc
