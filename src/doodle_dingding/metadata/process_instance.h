//
// Created by TD on 2022/9/9.
//

#pragma once

#include "doodle_core/metadata/time_point_wrap.h"

#include "configure/doodle_dingding_export.h"

#include <cstdint>
#include <string>
#include <vector>

namespace doodle::dingding::workflow_instances {
namespace deatil {
enum class approval_status : std::int16_t { NEW = 0, RUNNING, TERMINATED, COMPLETED, CANCELED };
enum class approval_result : std::int16_t { agree = 0, refuse };
enum class approval_action : std::int16_t { MODIFY = 0, REVOKE, NONE };
enum class task_status : std::int16_t { NEW = 0, RUNNING, PAUSED, CANCELED, COMPLETED, TERMINATED };
enum class task_result : std::int16_t { AGREE, REFUSE, REDIRECTED };

class form_component_values {
 public:
  std::string id;
  std::string name;
  std::string value;
  std::string extValue;
  std::string componentType;
  std::string bizAlias;

  friend void DOODLE_DINGDING_API
  to_json(nlohmann::json& nlohmann_json_j, const form_component_values& nlohmann_json_t);
  friend void DOODLE_DINGDING_API
  from_json(const nlohmann::json& nlohmann_json_j, form_component_values& nlohmann_json_t);
};

class task {
 public:
  std::string taskId;
  std::string userId;
  task_status status;
  task_result result;
  std::string createTime;
  std::string finishTime;
  std::string mobileUrl;
  std::string pcUrl;
  std::string processInstanceId;
  std::string activityId;

  friend void DOODLE_DINGDING_API to_json(nlohmann::json& nlohmann_json_j, const task& nlohmann_json_t);
  friend void DOODLE_DINGDING_API from_json(const nlohmann::json& nlohmann_json_j, task& nlohmann_json_t);
};

class attachment {
 public:
  std::string fileName;
  std::string fileSize;
  std::string fileId;
  std::string fileType;
  friend void DOODLE_DINGDING_API to_json(nlohmann::json& nlohmann_json_j, const attachment& nlohmann_json_t);
  friend void DOODLE_DINGDING_API from_json(const nlohmann::json& nlohmann_json_j, attachment& nlohmann_json_t);
};

class operation_records {
 public:
  std::string userId;
  time_point_wrap date;
  std::string type;
  std::string result;
  std::string remark;
  std::vector<attachment> attachments;

  friend void DOODLE_DINGDING_API to_json(nlohmann::json& nlohmann_json_j, const operation_records& nlohmann_json_t);
  friend void DOODLE_DINGDING_API from_json(const nlohmann::json& nlohmann_json_j, operation_records& nlohmann_json_t);
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
  std::string createTime{};

  friend void DOODLE_DINGDING_API to_json(nlohmann::json& nlohmann_json_j, const approval_form& nlohmann_json_t);
  friend void DOODLE_DINGDING_API from_json(const nlohmann::json& nlohmann_json_j, approval_form& nlohmann_json_t);
};
}  // namespace doodle::dingding::workflow_instances