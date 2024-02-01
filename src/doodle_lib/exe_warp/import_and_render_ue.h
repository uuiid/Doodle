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
  class wait_handle : public detail::wait_op {
   public:
    explicit wait_handle(Handler &&handler)
        : detail::wait_op(&wait_handle::on_complete, std::make_shared<Handler>(std::move(handler))) {}
    ~wait_handle() = default;
    FSys::path out_file_dir_{};
    static void on_complete(wait_op *op) {
      auto l_self = static_cast<wait_handle *>(op);
      boost::asio::post(boost::asio::prepend(
          std::move(*static_cast<Handler *>(l_self->handler_.get())), l_self->ec_, l_self->out_file_dir_
      ));
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
    std::string import_dir;

    std::string original_map;  // 地编提供的主场景路径, 我们需要抓取子场景

    FSys::path render_map;             // 渲染关卡, 这个放置外面, 包含下面两个子关卡
    std::string create_map;            // 创建的关卡(放置骨骼网格体)
    FSys::path vfx_map;                // 特效关卡, 放特效
    FSys::path level_sequence_import;  // 渲染关卡序列(包的路径), 包括下面的子关卡
    FSys::path level_sequence_vfx;     // 额外的特效关卡序列(包的路径)

    FSys::path movie_pipeline_config;  // 渲染配置(包的路径)

    std::vector<import_files_t> files;
    friend void to_json(nlohmann::json &j, const import_data_t &p) {
      j["project"]            = p.project_.p_shor_str;
      j["begin_time"]         = p.begin_time;
      j["end_time"]           = p.end_time;
      j["episode"]            = p.episode.p_episodes;
      j["shot"]               = p.shot.p_shot;
      j["shot_ab"]            = p.shot.p_shot_ab;
      j["out_file_dir"]       = p.out_file_dir.generic_string();
      j["original_map"]       = p.original_map;
      j["render_map"]         = p.render_map;
      j["files"]              = p.files;
      j["import_dir"]         = p.import_dir;
      j["create_map"]         = p.create_map;
      j["vfx_map"]            = p.vfx_map;
      j["level_sequence_vfx"] = p.level_sequence_vfx;

      auto l_path             = p.level_sequence_import;
      l_path.replace_extension();
      j["level_sequence"] = l_path;

      auto l_path2        = p.movie_pipeline_config;
      l_path2.replace_extension();
      j["movie_pipeline_config"] = l_path2;
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
  void fix_project() const;
  FSys::path gen_import_config() const;

 public:
  explicit import_and_render_ue(entt::handle in_msg) : data_(std::make_shared<data_impl_t>()), msg_(in_msg) { init(); }
  ~import_and_render_ue() = default;

  struct render_out_path_t {
    FSys::path path;
  };

  /**
   * @brief 异步结束的回调, 其中回调 path 为渲染输出路径, 格式是 .project 文件的父目录 / saved / movie_renders /
   * EP_****_sc_****
   * @tparam CompletionHandler  回调类型
   * @param handler  回调函数
   */
  template <typename CompletionHandler>
  auto async_end(CompletionHandler &&handler) {
    return boost::asio::async_initiate<CompletionHandler, void(boost::system::error_code, FSys::path)>(
        [this](auto &&handler) {
          wait_op_ =
              std::make_shared<wait_handle<std::decay_t<decltype(handler)>>>(std::forward<decltype(handler)>(handler));
          set_out_file_dir_ = [l_op = wait_op_, msg = msg_](FSys::path in_path) {
            auto l_wait_op           = std::static_pointer_cast<wait_handle<std::decay_t<decltype(handler)>>>(l_op);
            l_wait_op->out_file_dir_ = in_path;
            msg.emplace_or_replace<render_out_path_t>(in_path);
          };
        },
        handler
    );
  }

  void operator()(boost::system::error_code in_error_code, down_auto_light_anim_file::down_info in_down_info) const;
  void operator()(boost::system::error_code in_error_code) const;
};
}  // namespace doodle
