//
// Created by TD on 2022/9/9.
//

#pragma once

#include "doodle_core/metadata/time_point_wrap.h"

#include <string>
#include <vector>

namespace doodle::dingding::workflow_instances {
namespace deatil {
enum class approval_status { NEW = 0, RUNNING, TERMINATED, COMPLETED, CANCELED };
enum class approval_result { agree = 0, refuse };
enum class approval_action { MODIFY = 0, REVOKE, NONE };

class form_component_values {
 public:
  std::string id;
  std::string name;
  std::string value;
  std::string extValue;
  std::string componentType;
  std::string bizAlias;
};

class task {
 public:
  std::string taskId;
  std::string userId;
  std::string status;
  std::string result;
  std::string createTime;
  std::string finishTime;
  std::string mobileUrl;
  std::string pcUrl;
  std::string processInstanceId;
  std::string activityId;
};

class operation_records {
 public:
  std::string userId;
  std::string date;
  std::string type;
  std::string result;
  std::string remark;
  std::vector<std::string> attachments;
};

}  // namespace deatil

class approval_form {
 public:
  approval_form()  = default;
  ~approval_form() = default;

  std::string title{};
  std::string finishTime{};
  std::string originatorUserId{};
  std::string originatorDeptId{};
  std::string originatorDeptName{};
  deatil::approval_status status{};
  std::vector<std::string> approverUserIds{};
  std::vector<std::string> ccUserIds{};
  deatil::approval_result result{};
  std::string businessId{};

  std::vector<deatil::operation_records> operationRecords{};
  std::vector<deatil::task> tasks{};
  deatil::approval_action bizAction{};

  std::vector<std::string> attachedProcessInstanceIds{};
  std::string mainProcessInstanceId{};
  std::vector<deatil::form_component_values> formComponentValues{};
  time_point_wrap createTime{};
};
}  // namespace doodle::dingding::workflow_instances