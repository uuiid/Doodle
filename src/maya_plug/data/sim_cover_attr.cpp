
//
// Created by TD on 2022/1/7.
//

#include "sim_cover_attr.h"

#include <maya_plug/data/maya_tool.h>
#include <maya_plug/data/qcloth_shape.h>
#include <maya_plug/data/reference_file.h>

namespace doodle::maya_plug {
void to_json(nlohmann::json& j, const sim_cover_attr& p) {
  j["simple_subsampling"] = p.simple_subsampling;
  j["frame_samples"]      = p.frame_samples;
  j["time_scale"]         = p.time_scale;
  j["length_scale"]       = p.length_scale;
  j["max_cg_iteration"]   = p.max_cg_iteration;
  j["cg_accuracy"]        = p.cg_accuracy;
  j["gravity"]            = p.gravity;
}
void from_json(const nlohmann::json& j, sim_cover_attr& p) {
  if (j.contains("simple_subsampling"))
    j.at("simple_subsampling").get_to(p.simple_subsampling);
  if (j.contains("frame_samples"))
    j.at("frame_samples").get_to(p.frame_samples);
  if (j.contains("time_scale"))
    j.at("time_scale").get_to(p.time_scale);
  if (j.contains("length_scale"))
    j.at("length_scale").get_to(p.length_scale);
  if (j.contains("max_cg_iteration"))
    j.at("max_cg_iteration").get_to(p.max_cg_iteration);
  if (j.contains("cg_accuracy"))
    j.at("cg_accuracy").get_to(p.cg_accuracy);
  if (j.contains("gravity"))
    j.at("gravity").get_to(p.gravity);
}
void sim_cover_attr::cover_qcloth_attr(const entt::handle& in_handle) {
  if (in_handle && in_handle.all_of<sim_cover_attr, reference_file>()) {
    DOODLE_LOG_INFO("开始覆盖 {} 的解算配置", in_handle.get<reference_file>().path);
    auto& self     = in_handle.get<sim_cover_attr>();
    auto l_ql_core = qcloth_shape::get_ql_solver(
        in_handle.get<reference_file>().get_all_object()
    );
    set_attribute(l_ql_core, "simpleSubsampling", self.simple_subsampling);
    DOODLE_LOG_INFO("开始覆盖 simpleSubsampling 值为 {}", self.simple_subsampling);
    set_attribute(l_ql_core, "frameSamples", self.frame_samples);
    DOODLE_LOG_INFO("开始覆盖 frameSamples 值为 {}", self.frame_samples);
    set_attribute(l_ql_core, "timeScale", self.time_scale);
    DOODLE_LOG_INFO("开始覆盖 timeScale 值为 {}", self.time_scale);
    set_attribute(l_ql_core, "lengthScale", self.length_scale);
    DOODLE_LOG_INFO("开始覆盖 lengthScale 值为 {}", self.length_scale);
    set_attribute(l_ql_core, "maxCGIteration", self.max_cg_iteration);
    DOODLE_LOG_INFO("开始覆盖 maxCGIteration 值为 {}", self.max_cg_iteration);
    set_attribute(l_ql_core, "cgAccuracy", self.cg_accuracy);
    DOODLE_LOG_INFO("开始覆盖 cgAccuracy 值为 {}", self.cg_accuracy);
    set_attribute(l_ql_core, "gravity0", self.gravity[0]);
    set_attribute(l_ql_core, "gravity1", self.gravity[1]);
    set_attribute(l_ql_core, "gravity2", self.gravity[2]);
    DOODLE_LOG_INFO("开始覆盖 gravity 值为 {}", self.gravity);
  }
}

}  // namespace doodle::maya_plug
