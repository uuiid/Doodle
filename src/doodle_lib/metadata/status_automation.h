#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_core/metadata/status_automation.h>

#include <boost/asio/awaitable.hpp>

namespace doodle::status_automation_ns {
boost::asio::awaitable<void> run(
    const status_automation& in_status_automation, const std::shared_ptr<task>& in_task, const uuid& in_person_id
);

}