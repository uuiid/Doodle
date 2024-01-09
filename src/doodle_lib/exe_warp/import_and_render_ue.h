//
// Created by TD on 2024/1/8.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/shot.h>

#include <doodle_lib/core/down_auto_light_anim_file.h>

namespace doodle {

class import_and_render_ue {
  template <typename Handler>
  class wait_handle : detail::wait_op {
   public:
    explicit wait_handle(Handler &&handler)
        : detail::wait_op(&wait_handle::on_complete, std::make_shared<Handler>(handler)) {}
    ~wait_handle() = default;
    FSys::path out_file_dir_{};
    static void on_complete(wait_op *op) {
      auto l_self = static_cast<wait_handle *>(op);
      boost::asio::post(boost::asio::prepend(std::move(*static_cast<Handler *>(l_self->handler_.get())), l_self->ec_));
    }
  };

  struct import_files_t {
    std::string type_;
    FSys::path path_;
    friend void to_json(nlohmann::json &j, const import_files_t &p) {
      j["type"] = p.type_;
      j["path"] = p.path_.generic_string();
    }
  };
  struct import_data_t {
    project project_;
    std::int32_t begin_time;
    std::int32_t end_time;
    episodes episode;
    shot shot;

    FSys::path out_file_dir;
    std::string original_map;
    std::string import_dir;
    std::string create_map;

    FSys::path render_map;             // 渲染关卡
    FSys::path level_sequence;         // 渲染关卡序列
    FSys::path movie_pipeline_config;  // 渲染配置

    std::vector<import_files_t> files;
    friend void to_json(nlohmann::json &j, const import_data_t &p) {
      j["project"]               = p.project_.p_shor_str;
      j["begin_time"]            = p.begin_time;
      j["end_time"]              = p.end_time;
      j["episode"]               = p.episode.p_episodes;
      j["shot"]                  = p.shot.p_shot;
      j["shot_ab"]               = p.shot.p_shot_ab;
      j["out_file_dir"]          = p.out_file_dir.generic_string();
      j["original_map"]          = p.original_map;
      j["render_map"]            = p.render_map;
      j["files"]                 = p.files;
      j["import_dir"]            = p.import_dir;
      j["level_sequence"]        = p.level_sequence;
      j["create_map"]            = p.create_map;
      j["movie_pipeline_config"] = p.movie_pipeline_config;
    }
  };

  struct args {
    FSys::path ue_project_path_{};

    import_data_t import_data_{};
  };

  enum class status {
    import_file,
    render,
  };

  struct data_impl_t {
    logger_ptr logger_{};
    down_auto_light_anim_file::down_info down_info_{};
    import_data_t import_data_{};
    status status_{status::import_file};
  };
  std::shared_ptr<data_impl_t> data_{};  // 用于存储数据

  entt::handle msg_{};
  std::shared_ptr<detail::wait_op> wait_op_{};  // 回调
  std::function<void(FSys::path)> set_out_file_dir_{};
  void init();
  FSys::path gen_import_config() const;

 public:
  explicit import_and_render_ue(entt::handle in_msg) : data_(std::make_shared<data_impl_t>()), msg_(in_msg) { init(); }
  ~import_and_render_ue() = default;

  template <typename CompletionHandler>
  auto async_end(CompletionHandler &&handler) {
    return boost::asio::async_initiate<CompletionHandler, void(boost::system::error_code)>(
        [this](auto &&handler) {
          wait_op_ =
              std::make_shared<wait_handle<std::decay_t<decltype(handler)>>>(std::forward<decltype(handler)>(handler));
          set_out_file_dir_ = [this](FSys::path in_path) {
            auto l_wait_op = std::dynamic_pointer_cast<wait_handle<std::decay_t<decltype(handler)>>>(wait_op_);
          }
        },
        wait_op_
    );
  }

  void operator()(boost::system::error_code in_error_code, down_auto_light_anim_file::down_info &in_down_info) const;
  void operator()(boost::system::error_code in_error_code) const;
};
}  // namespace doodle
