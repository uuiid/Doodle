//
// Created by TD on 2022/4/28.
//
#pragma once
#include <doodle_core/configure/config.h>
#include <doodle_core/configure/doodle_core_export.h>
#include <doodle_core/configure/static_value.h>
#include <doodle_core/doodle_core_pch.h>
#include <doodle_core/doodle_macro.h>
#include <doodle_core/exception/exception.h>
#include <doodle_core/lib_warp/json_warp.h>

#include <boost/uuid/uuid.hpp>

#include <entt/entt.hpp>

namespace boost::asio {
class io_context;
class thread_pool;
}  // namespace boost::asio

namespace doodle::chrono {
using namespace std::chrono;
namespace literals {
using namespace std::chrono_literals;
}  // namespace literals
using namespace std::chrono;

using hours_double      = duration<std::double_t, std::ratio<3600>>;
using sys_time_pos      = time_point<system_clock>;
using local_time_pos    = time_point<local_t, seconds>;
using system_zoned_time = zoned_time<system_clock::duration>;
};  // namespace doodle::chrono

namespace doodle::FSys {
using namespace std::filesystem;
using fstream  = std::fstream;
using istream  = std::istream;
using ifstream = std::ifstream;
using ofstream = std::ofstream;
using ostream  = std::ostream;
}  // namespace doodle::FSys

// #include <>
namespace doodle {

class doodle_error;

using uuid = boost::uuids::uuid;

using namespace std::literals;

};  // namespace doodle
