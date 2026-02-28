#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/entity.h>

namespace doodle::entity_ns {
std::tuple<std::string, uuid> get_full_name(const entity& in_entity);

}