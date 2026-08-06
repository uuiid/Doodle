#include "doodle_core/exception/exception.h"

#include <doodle_lib/ai/cfg_type.h>
#include <doodle_lib/ai/kimodo.h>
#include <doodle_lib/ai/motion_rep/constraint_set.h>
#include <doodle_lib/core/global_function.h>
#include <doodle_lib/http_method/ai/ai_main.h>

#include <boost/asio/post.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <memory>
#include <mutex>
#include <onnxruntime_cxx_api.h>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

namespace doodle::http {
namespace {
// 初始化 onnxruntime 环境
void _init_ort_env() {
  try {
    auto env                         = std::make_shared<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "doodle_ort");
    core_set::get_set().ort_env_ptr_ = env;
    SPDLOG_INFO("ONNX Runtime 环境初始化成功");
  } catch (const Ort::Exception& e) {
    SPDLOG_ERROR("ONNX Runtime 环境初始化失败: {}", e.what());
  }
}
void init_ort_env() {
  static std::once_flag l_flag{};
  std::call_once(l_flag, &_init_ort_env);
}

}  // namespace

struct ai_train_animation::impl {
  std::shared_ptr<doodle::ai::kimodo> model_{};
  impl() = default;
  void init(const std::string& model_path) {
    model_        = std::make_shared<doodle::ai::kimodo>();
    auto l_config = std::make_shared<doodle::ai::kimodo_model_config>();
    l_config->load_from_json(model_path);
    model_->load(l_config);
  }
  auto run(const doodle::ai::generate_arg& segments) {
    DOODLE_CHICK(model_ != nullptr, "模型未加载，请先调用 load_model");
    return model_->generate(segments);
  }
};
ai_train_animation::ai_train_animation() : impl_ptr_(std::make_shared<impl>()) {}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(ai_train_animation, post) {
  DOODLE_CHICK(impl_ptr_->model_, "模型为空，请先调用 load_model");
  DOODLE_CHICK(impl_ptr_->model_->is_valid(), "模型加载错误，请先调用 load_model");

  doodle::ai::generate_arg l_segment = in_handle->get_json().get<doodle::ai::generate_arg>();
  DOODLE_CHICK(!l_segment.segments_.empty(), "segment 不能为空");

  for (auto& seg : l_segment.segments_) {
    if (seg.constraint_lst_.is_array() && !seg.constraint_lst_.empty()) {
      seg.constraints_ = doodle::ai::load_constraints_lst_from_json(seg.constraint_lst_, impl_ptr_->model_->skeleton());
    }
  }
  // 获取根节点的位置
  if (l_segment.skeleton_) {
    auto l_root_index = impl_ptr_->model_->skeleton()->root_idx_;
    DOODLE_CHICK(
        l_segment.skeleton_->neutral_joints_.row(l_root_index).head<3>() == (Eigen::Vector3f{0, 0, 0}), "根节点位置不为零"
    );
    // auto l_neutral_joints =
    //     impl_ptr_->model_->motion_rep()->translate_2d_to_zero(l_segment.skeleton_->neutral_joints_, 1, 1);
    // auto l_root_pos =
    //     l_segment.skeleton_->neutral_joints_.row(l_root_index).head<3>() -
    //     l_neutral_joints.row(l_root_index).head<3>();
    // l_segment.skeleton_->neutral_joints_ = l_neutral_joints;
    // l_segment.root_trajectory_ += l_root_pos;
  }
  co_return in_handle->make_msg(nlohmann::json{} = impl_ptr_->run(l_segment));
}

void ai_train_animation::load_model(const std::string& model_path) {
  init_ort_env();
  impl_ptr_->init(model_path);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(ai_train_animation_settings, post) {
  auto l_path     = in_handle->get_json().at("model_path").get<std::string>();
  auto l_main_fun = in_handle->route_ptr_->get_function<ai_train_animation>();
  DOODLE_CHICK(l_main_fun != nullptr, "ai_train_animation 路由未注册，请检查代码");
  l_main_fun->load_model(l_path);
  co_return in_handle->make_msg(
      nlohmann::json{{"model_path", l_path}, {"skeleton", *l_main_fun->impl_ptr_->model_->skeleton()}}
  );
}
}  // namespace doodle::http