#include "process_instance.h"

#include "metadata/dingding_tool.h"
#include <magic_enum.hpp>
#include <string>

namespace doodle::dingding::workflow_instances {
namespace deatil {

void to_json(nlohmann::json& nlohmann_json_j, const form_component_values& nlohmann_json_t) {
  nlohmann_json_j["id"]            = nlohmann_json_t.id;
  nlohmann_json_j["name"]          = nlohmann_json_t.name;
  nlohmann_json_j["value"]         = nlohmann_json_t.value;
  nlohmann_json_j["extValue"]      = nlohmann_json_t.extValue;
  nlohmann_json_j["componentType"] = nlohmann_json_t.componentType;
  nlohmann_json_j["bizAlias"]      = nlohmann_json_t.bizAlias;
}
void from_json(const nlohmann::json& nlohmann_json_j, form_component_values& nlohmann_json_t) {
  nlohmann_json_j.at("id").get_to(nlohmann_json_t.id);
  nlohmann_json_j.at("name").get_to(nlohmann_json_t.name);
  nlohmann_json_j.at("value").get_to(nlohmann_json_t.value);
  nlohmann_json_j.at("extValue").get_to(nlohmann_json_t.extValue);
  nlohmann_json_j.at("componentType").get_to(nlohmann_json_t.componentType);
  nlohmann_json_j.at("bizAlias").get_to(nlohmann_json_t.bizAlias);
}

void to_json(nlohmann::json& nlohmann_json_j, const task& nlohmann_json_t) {
  nlohmann_json_j["taskId"]            = nlohmann_json_t.taskId;
  nlohmann_json_j["userId"]            = nlohmann_json_t.userId;
  nlohmann_json_j["status"]            = nlohmann_json_t.status;
  nlohmann_json_j["result"]            = nlohmann_json_t.result;
  nlohmann_json_j["createTime"]        = nlohmann_json_t.createTime;
  nlohmann_json_j["finishTime"]        = nlohmann_json_t.finishTime;
  nlohmann_json_j["mobileUrl"]         = nlohmann_json_t.mobileUrl;
  nlohmann_json_j["pcUrl"]             = nlohmann_json_t.pcUrl;
  nlohmann_json_j["processInstanceId"] = nlohmann_json_t.processInstanceId;
  nlohmann_json_j["activityId"]        = nlohmann_json_t.activityId;
}
void from_json(const nlohmann::json& nlohmann_json_j, task& nlohmann_json_t) {
  nlohmann_json_j.at("taskId").get_to(nlohmann_json_t.taskId);
  nlohmann_json_j.at("userId").get_to(nlohmann_json_t.userId);

  if (auto l_e =
          magic_enum::enum_cast<decltype(nlohmann_json_t.status)>(nlohmann_json_j.at("status").get<std::string>()))
    nlohmann_json_t.status = *l_e;

  if (auto l_e =
          magic_enum::enum_cast<decltype(nlohmann_json_t.result)>(nlohmann_json_j.at("result").get<std::string>()))
    nlohmann_json_t.result = *l_e;

  nlohmann_json_j.at("createTime").get_to(nlohmann_json_t.createTime);
  nlohmann_json_j.at("finishTime").get_to(nlohmann_json_t.finishTime);
  nlohmann_json_j.at("mobileUrl").get_to(nlohmann_json_t.mobileUrl);
  nlohmann_json_j.at("pcUrl").get_to(nlohmann_json_t.pcUrl);
  nlohmann_json_j.at("processInstanceId").get_to(nlohmann_json_t.processInstanceId);
  nlohmann_json_j.at("activityId").get_to(nlohmann_json_t.activityId);
}

void to_json(nlohmann::json& nlohmann_json_j, const attachment& nlohmann_json_t) {
  nlohmann_json_j["fileName"] = nlohmann_json_t.fileName;
  nlohmann_json_j["fileSize"] = nlohmann_json_t.fileSize;
  nlohmann_json_j["fileId"]   = nlohmann_json_t.fileId;
  nlohmann_json_j["fileType"] = nlohmann_json_t.fileType;
}
void from_json(const nlohmann::json& nlohmann_json_j, attachment& nlohmann_json_t) {
  nlohmann_json_j.at("fileName").get_to(nlohmann_json_t.fileName);
  nlohmann_json_j.at("fileSize").get_to(nlohmann_json_t.fileSize);
  nlohmann_json_j.at("fileId").get_to(nlohmann_json_t.fileId);
  nlohmann_json_j.at("fileType").get_to(nlohmann_json_t.fileType);
}

void to_json(nlohmann::json& nlohmann_json_j, const operation_records& nlohmann_json_t) {
  nlohmann_json_j["userId"] = nlohmann_json_t.userId;
  nlohmann_json_j["date"]   = nlohmann_json_t.date;
  nlohmann_json_j["type"]   = nlohmann_json_t.type;
  nlohmann_json_j["result"] = nlohmann_json_t.result;
  nlohmann_json_j["remark"] = nlohmann_json_t.remark;
}
void from_json(const nlohmann::json& nlohmann_json_j, operation_records& nlohmann_json_t) {
  nlohmann_json_j.at("userId").get_to(nlohmann_json_t.userId);
  nlohmann_json_t.date = dingding::detail::tool::parse_dingding_Date(nlohmann_json_j.at("date"));
  nlohmann_json_j.at("type").get_to(nlohmann_json_t.type);
  nlohmann_json_j.at("result").get_to(nlohmann_json_t.result);
  nlohmann_json_j.at("remark").get_to(nlohmann_json_t.remark);
}
}  // namespace deatil

void to_json(nlohmann::json& nlohmann_json_j, const approval_form& nlohmann_json_t) {
  nlohmann_json_j["title"]                      = nlohmann_json_t.title;
  nlohmann_json_j["finishTime"]                 = nlohmann_json_t.finishTime;
  nlohmann_json_j["originatorUserId"]           = nlohmann_json_t.originatorUserId;
  nlohmann_json_j["originatorDeptId"]           = nlohmann_json_t.originatorDeptId;
  nlohmann_json_j["originatorDeptName"]         = nlohmann_json_t.originatorDeptName;
  nlohmann_json_j["status"]                     = nlohmann_json_t.status;
  nlohmann_json_j["approverUserIds"]            = nlohmann_json_t.approverUserIds;
  nlohmann_json_j["ccUserIds"]                  = nlohmann_json_t.ccUserIds;
  nlohmann_json_j["result"]                     = nlohmann_json_t.result;
  nlohmann_json_j["businessId"]                 = nlohmann_json_t.businessId;
  nlohmann_json_j["operationRecords"]           = nlohmann_json_t.operationRecords;
  nlohmann_json_j["tasks"]                      = nlohmann_json_t.tasks;
  nlohmann_json_j["bizAction"]                  = nlohmann_json_t.bizAction;
  nlohmann_json_j["attachedProcessInstanceIds"] = nlohmann_json_t.attachedProcessInstanceIds;
  nlohmann_json_j["mainProcessInstanceId"]      = nlohmann_json_t.mainProcessInstanceId;
  nlohmann_json_j["formComponentValues"]        = nlohmann_json_t.formComponentValues;
  nlohmann_json_j["createTime"]                 = nlohmann_json_t.createTime;
}
void from_json(const nlohmann::json& nlohmann_json_j, approval_form& nlohmann_json_t) {
  nlohmann_json_j.at("title").get_to(nlohmann_json_t.title);
  nlohmann_json_j.at("finishTime").get_to(nlohmann_json_t.finishTime);
  nlohmann_json_j.at("originatorUserId").get_to(nlohmann_json_t.originatorUserId);
  nlohmann_json_j.at("originatorDeptId").get_to(nlohmann_json_t.originatorDeptId);
  nlohmann_json_j.at("originatorDeptName").get_to(nlohmann_json_t.originatorDeptName);

  if (auto l_e =
          magic_enum::enum_cast<decltype(nlohmann_json_t.status)>(nlohmann_json_j.at("status").get<std::string>()))
    nlohmann_json_t.status = *l_e;

  nlohmann_json_j.at("approverUserIds").get_to(nlohmann_json_t.approverUserIds);
  nlohmann_json_j.at("ccUserIds").get_to(nlohmann_json_t.ccUserIds);

  if (auto l_e =
          magic_enum::enum_cast<decltype(nlohmann_json_t.result)>(nlohmann_json_j.at("result").get<std::string>()))
    nlohmann_json_t.result = *l_e;

  nlohmann_json_j.at("businessId").get_to(nlohmann_json_t.businessId);
  nlohmann_json_j.at("operationRecords").get_to(nlohmann_json_t.operationRecords);
  nlohmann_json_j.at("tasks").get_to(nlohmann_json_t.tasks);

  if (auto l_e =
          magic_enum::enum_cast<decltype(nlohmann_json_t.bizAction)>(nlohmann_json_j.at("bizAction").get<std::string>()
          ))
    nlohmann_json_t.bizAction = *l_e;

  nlohmann_json_j.at("attachedProcessInstanceIds").get_to(nlohmann_json_t.attachedProcessInstanceIds);
  nlohmann_json_j.at("mainProcessInstanceId").get_to(nlohmann_json_t.mainProcessInstanceId);
  nlohmann_json_j.at("formComponentValues").get_to(nlohmann_json_t.formComponentValues);
  nlohmann_json_j.at("createTime").get_to(nlohmann_json_t.createTime);
}

}  // namespace doodle::dingding::workflow_instances