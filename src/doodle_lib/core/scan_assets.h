//
// Created by TD on 25-8-13.
//

#pragma once
#include <doodle_core/metadata/asset_instance.h>
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/working_file.h>

#include <memory>
namespace doodle {
struct task;
}
namespace doodle::scan_assets {

class scan_result {
 public:
  std::vector<working_file> working_files_;
  std::vector<working_file_entity_link> working_file_entity_links_;
  std::vector<working_file_task_link> working_file_task_links_;
  scan_result() = default;

  boost::asio::awaitable<void> install_async_sqlite();
  // operator +=
  void operator+=(const scan_result& in) {
    working_files_.insert(working_files_.end(), in.working_files_.begin(), in.working_files_.end());
    working_file_entity_links_.insert(working_file_entity_links_.end(), in.working_file_entity_links_.begin(),
                                     in.working_file_entity_links_.end());
    working_file_task_links_.insert(working_file_task_links_.end(), in.working_file_task_links_.begin(),
                                   in.working_file_task_links_.end());
  }

};
FSys::path scan_maya(const project& in_prj, const uuid& in_entity_type, const entity_asset_extend& in_extend);
FSys::path scan_unreal_engine(const project& in_prj, const uuid& in_entity_type, const entity_asset_extend& in_extend);
FSys::path scan_rig_maya(const project& in_prj, const uuid& in_entity_type, const entity_asset_extend& in_extend);
FSys::path scan_sim_maya(const project& in_prj, const working_file& in_extend);

boost::asio::awaitable<std::shared_ptr<scan_result>> scan_task_async(const task& in_task);
std::shared_ptr<scan_result> scan_task(const task& in_task);

}  // namespace doodle::scan_assets