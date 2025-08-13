//
// Created by TD on 25-8-13.
//

#pragma once
#include <doodle_core/metadata/asset_instance.h>
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/working_file.h>
namespace doodle::scan_assets {

std::shared_ptr<working_file> scan_maya(
    const project& in_prj, const uuid& in_entity_type, const entity_asset_extend& in_extend
);
std::shared_ptr<working_file> scan_unreal_engine(
    const project& in_prj, const uuid& in_entity_type, const entity_asset_extend& in_extend
);
}