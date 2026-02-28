#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_core/metadata/entity.h>

namespace doodle::entity_ns {
std::tuple<std::string, uuid> get_full_name(const entity& in_entity);

}