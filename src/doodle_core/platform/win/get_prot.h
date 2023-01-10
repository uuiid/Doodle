//
// Created by TD on 2022/10/8.
//
#pragma once

#include <doodle_core/doodle_core_fwd.h>
namespace doodle::win {

std::uint32_t DOODLE_CORE_API get_tcp_port(std::uint32_t id);
bool DOODLE_CORE_API has_tcp_port(std::uint32_t in_port);
}
