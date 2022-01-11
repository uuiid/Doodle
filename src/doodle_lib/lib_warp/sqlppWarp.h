//
// Created by TD on 2021/6/17.
//

#pragma once
// 添加数据库连接项
namespace sqlpp {
namespace mysql {
class connection;
struct connection_config;
}  // namespace mysql

namespace sqlite3{
class connection;
struct connection_config;
}
}  // namespace sqlpp
