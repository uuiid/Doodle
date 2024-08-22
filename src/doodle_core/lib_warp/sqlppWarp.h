//
// Created by TD on 2021/6/17.
//

#pragma once
#include <sqlpp11/connection.h>
// 添加数据库连接项
namespace sqlpp {
template <typename ConnectionBase>
class normal_connection;
template <typename ConnectionBase>
class pooled_connection;
template <typename ConnectionBase>
class connection_pool;
template <typename ConnectionBase>
class common_connection;

namespace sqlite3 {
class connection_base;
using common_connection = common_connection<sqlite3::connection_base>;
using connection        = normal_connection<sqlite3::connection_base>;
using pooled_connection = sqlpp::pooled_connection<sqlite3::connection_base>;
using connection_pool   = sqlpp::connection_pool<sqlite3::connection_base>;
}  // namespace sqlite3
}  // namespace sqlpp
