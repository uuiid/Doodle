//
// Created by TD on 2021/6/28.
//

#include "assets_file_action.h"

#include <Metadata/AssetsFile.h>
#include <Metadata/Assets.h>
#include <Metadata/AssetsPath.h>
#include <Metadata/Comment.h>
#include <Metadata/TimeDuration.h>
namespace doodle {

assfile_create_action::assfile_create_action(std::any&& in_any) : action(std::move(in_any)) {}
void assfile_create_action::run(const MetadataPtr& in_data) {
  if (!p_any.has_value())
    p_any = sig_get_input().value();


}

assfile_add_com_action::assfile_add_com_action(std::any&& in_any) : action(std::move(in_any)) {}
void assfile_add_com_action::run(const MetadataPtr& in_data) {
  if (!p_any.has_value())
    p_any = sig_get_input().value();
}

assfile_datetime_action::assfile_datetime_action(std::any&& in_any) : action(std::move(in_any)) {}
void assfile_datetime_action::run(const MetadataPtr& in_data) {
  if (!p_any.has_value())
    p_any = sig_get_input().value();
}

assfile_delete_action::assfile_delete_action(std::any&& in_any) : action(std::move(in_any)) {}
void assfile_delete_action::run(const MetadataPtr& in_data) {

}
}  // namespace doodle
