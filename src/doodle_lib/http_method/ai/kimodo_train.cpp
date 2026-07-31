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

struct ai_train_binding_weights_post_args {
  std::vector<std::string> text_{};
  std::vector<std::int64_t> num_frames_{120};
  std::int32_t num_denoising_steps_{50};
  std::vector<std::float_t> cfg_weight_{80, 80};
  std::int32_t num_samples_{100};
  doodle::ai::cfg_type cfg_type_{doodle::ai::cfg_type::separated};
  std::vector<std::float_t> first_heading_angle_{0.0f};
  std::int32_t num_transition_frames_{10};
  bool post_processing_{true};
  std::float_t root_margin_{0.0f};
  nlohmann::json constraint_lst_{};
  std::vector<std::vector<doodle::ai::constraint_set_ptr>> constraints_per_sample_{};
  // from json
  friend void from_json(const nlohmann::json& in_json, ai_train_binding_weights_post_args& out) {
    if (in_json.contains("text") && in_json.at("text").is_string()) {
      out.text_.resize(1);
      in_json.at("text").get_to(out.text_.front());
    } else if (in_json.contains("text") && in_json.at("text").is_array() && in_json.at("text").size() > 0 &&
               in_json.at("text").at(0).is_string()) {
      in_json.at("text").get_to(out.text_);
    }
    if (in_json.contains("num_frames") && in_json.at("num_frames").is_number_integer()) {
      out.num_frames_.resize(1);
      in_json.at("num_frames").get_to(out.num_frames_.front());
    } else if (in_json.contains("num_frames") && in_json.at("num_frames").is_array() &&
               in_json.at("num_frames").size() > 0 && in_json.at("num_frames").at(0).is_number_integer()) {
      in_json.at("num_frames").get_to(out.num_frames_);
    }
    if (in_json.contains("num_denoising_steps") && in_json.at("num_denoising_steps").is_number_integer()) {
      in_json.at("num_denoising_steps").get_to(out.num_denoising_steps_);
    }
    if (in_json.contains("cfg_weight") && in_json.at("cfg_weight").is_array() && in_json.at("cfg_weight").size() > 0 &&
        in_json.at("cfg_weight").at(0).is_number_integer()) {
      in_json.at("cfg_weight").get_to(out.cfg_weight_);
    }
    if (in_json.contains("num_samples") && in_json.at("num_samples").is_number_integer()) {
      in_json.at("num_samples").get_to(out.num_samples_);
    }
    if (in_json.contains("cfg_type") && in_json.at("cfg_type").is_string()) {
      in_json.at("cfg_type").get_to(out.cfg_type_);
    }
    if (in_json.contains("first_heading_angle") && in_json.at("first_heading_angle").is_number_float()) {
      out.first_heading_angle_.resize(1);
      in_json.at("first_heading_angle").get_to(out.first_heading_angle_.front());
    } else if (in_json.contains("first_heading_angle") && in_json.at("first_heading_angle").is_array() &&
               in_json.at("first_heading_angle").size() > 0 &&
               in_json.at("first_heading_angle").at(0).is_number_float()) {
      in_json.at("first_heading_angle").get_to(out.first_heading_angle_);
    }
    if (in_json.contains("num_transition_frames") && in_json.at("num_transition_frames").is_number_integer()) {
      in_json.at("num_transition_frames").get_to(out.num_transition_frames_);
    }
    if (in_json.contains("post_processing") && in_json.at("post_processing").is_boolean()) {
      in_json.at("post_processing").get_to(out.post_processing_);
    }
    if (in_json.contains("root_margin") && in_json.at("root_margin").is_number_float()) {
      in_json.at("root_margin").get_to(out.root_margin_);
    }
    if (in_json.contains("constraint_lst") && in_json.at("constraint_lst").is_array()) {
      in_json.at("constraint_lst").get_to(out.constraint_lst_);
    }
  }
};

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
  auto run(const ai_train_binding_weights_post_args& in_args) {
    DOODLE_CHICK(model_ != nullptr, "模型未加载，请先调用 load_model");
    auto l_args = in_args;
    if (l_args.constraint_lst_.is_array() && l_args.constraint_lst_.size() > 0 &&
        l_args.constraint_lst_.at(0).is_array()) {
      l_args.constraints_per_sample_.resize(l_args.constraint_lst_.size());
      for (std::size_t i = 0; i < l_args.constraint_lst_.size(); ++i) {
        l_args.constraints_per_sample_[i] =
            doodle::ai::load_constraints_lst_from_json(l_args.constraint_lst_.at(i), model_->skeleton());
      }
    }
    DOODLE_CHICK(
        l_args.constraints_per_sample_.empty() || l_args.constraints_per_sample_.size() == l_args.text_.size(),
        "约束列表数量 ({}) 与文本提示数量 ({}) 不匹配", l_args.constraints_per_sample_.size(), l_args.text_.size()
    );

    auto output = model_->generate(
        l_args.text_, l_args.num_frames_, l_args.num_denoising_steps_, l_args.cfg_weight_,
        {l_args.first_heading_angle_}, l_args.constraints_per_sample_, l_args.cfg_type_
    );
    return output;
  }
};
ai_train_animation::ai_train_animation() : impl_ptr_(std::make_shared<impl>()) {}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(ai_train_animation, post) {
  // 这里推理是在本地进行, 只需要阻塞当前协程,
  // todo: 未来需要使 ::doodle ::http ::http_function 继承的类可以指定协程调度器, 以便在协程中使用 co_await
  // 来等待阻塞操作完成
  auto l_args = in_handle->get_json().get<ai_train_binding_weights_post_args>();
  co_return in_handle->make_msg(nlohmann::json{} = impl_ptr_->run(l_args));
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
  co_return in_handle->make_msg(nlohmann::json{{"model_path", l_path}});
}
}  // namespace doodle::http