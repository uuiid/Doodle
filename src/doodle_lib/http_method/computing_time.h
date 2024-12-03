#pragma once

#include "doodle_core/metadata/user.h"

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>
namespace doodle::http {
boost::asio::awaitable<tl::expected<void, std::string>> recomputing_time(
    const std::shared_ptr<user_helper::database_t>& in_user, const chrono::year_month& in_year_month
);
void reg_computing_time(http_route& in_route);
}  // namespace doodle::http