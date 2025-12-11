//
// Created by TD on 25-5-8.
//

#pragma once
#include <doodle_lib/http_method/http_jwt_fun.h>

#include <utility>

namespace doodle::http::model_library {
// "api/doodle/model_library/context"
DOODLE_HTTP_JWT_FUN(context)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// "api/doodle/model_library/assets"
DOODLE_HTTP_JWT_FUN(model_library_assets)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_END()

// "/api/doodle/ai_image"
// 用于管理 AI 生成图片的元数据（创建/查询）
DOODLE_HTTP_JWT_FUN(ai_image)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post);
DOODLE_HTTP_FUN_END()
// "/api/doodle/ai_image/{id}"
DOODLE_HTTP_JWT_FUN(ai_image_instance)
DOODLE_HTTP_FUN_OVERRIDE(delete_);
uuid id_{};
DOODLE_HTTP_FUN_END()

// "api/doodle/model_library/assets/{id}"
DOODLE_HTTP_JWT_FUN(model_library_assets_instance)
DOODLE_HTTP_FUN_OVERRIDE(put);
DOODLE_HTTP_FUN_OVERRIDE(delete_);
uuid id_{};
DOODLE_HTTP_FUN_END()

// "api/doodle/model_library/assets_tree"
DOODLE_HTTP_JWT_FUN(model_library_assets_tree)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_OVERRIDE(patch)
DOODLE_HTTP_FUN_END()
// "api/doodle/model_library/assets_tree/{id}"
DOODLE_HTTP_JWT_FUN(model_library_assets_tree_instance)
DOODLE_HTTP_FUN_OVERRIDE(put);
DOODLE_HTTP_FUN_OVERRIDE(delete_);
uuid id_{};
DOODLE_HTTP_FUN_END()

class pictures_base : public http_jwt_fun {
 protected:
  std::pair<std::size_t, std::size_t> create_thumbnail_mp4(
      const FSys::path& in_data_path, const FSys::path& in_path, FSys::path in_name
  );
  std::pair<std::size_t, std::size_t> create_thumbnail_gif(
      const FSys::path& in_data_path, const FSys::path& in_path, FSys::path in_name
  );
  std::pair<std::size_t, std::size_t> create_thumbnail_image(
      const std::string& in_data, const FSys::path& in_path, FSys::path in_name
  );
  boost::asio::awaitable<boost::beast::http::message_generator> thumbnail_get(
      session_data_ptr in_handle, FSys::path in_path, std::string in_extension = ".png"
  );
  boost::asio::awaitable<boost::beast::http::message_generator> thumbnail_post(
      session_data_ptr in_handle, FSys::path in_path
  );
  boost::asio::awaitable<boost::beast::http::message_generator> thumbnail_404(session_data_ptr in_handle);
  std::shared_ptr<FSys::path> root_;

 public:
  explicit pictures_base(const FSys::path& in_root) : root_(std::make_shared<FSys::path>(in_root)) {}
  uuid id_{};
};

// "api/doodle/pictures/{id}"
DOODLE_HTTP_FUN_C(pictures_instance, pictures_base)
explicit pictures_instance(const FSys::path& in_root) : base_type(in_root) {}
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post);
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN_C(pictures_instance_mp4, pictures_base)
explicit pictures_instance_mp4(const FSys::path& in_root) : base_type(in_root) {}
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()


// "api/doodle/pictures/thumbnails/{id}"
DOODLE_HTTP_FUN_C(pictures_thumbnails, pictures_base)
explicit pictures_thumbnails(const FSys::path& in_root) : base_type(in_root) {}
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// "api/doodle/model_library/assets_tree/{id}/assets/{assets_id}"
DOODLE_HTTP_JWT_FUN(assets_tree_link)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid id_;
uuid assets_id_;
DOODLE_HTTP_FUN_END()

}  // namespace doodle::http::model_library
