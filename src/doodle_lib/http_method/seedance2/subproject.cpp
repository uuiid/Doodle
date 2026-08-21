#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/metadata/person.h"
#include "doodle_core/metadata/seedance2/ai_preview_file.h"
#include <doodle_core/metadata/kitsu_ctx_t.h>
#include <doodle_core/metadata/seedance2/subproject.h>

#include <doodle_lib/http_method/seedance2/reg.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include "core/global_function.h"
#include "reg.h"
#include <map>
#include <memory>
#include <opencv2/opencv.hpp>

namespace doodle::http::seedance2 {
namespace sd2 = doodle::seedance2;

namespace {
struct seedance2_subproject_and_person : public sd2::subproject {
  std::vector<uuid> persons_;
  sd2::ai_preview_file preview_file_;
  explicit seedance2_subproject_and_person(const sd2::subproject& in_subproject, const sd2::ai_preview_file& in_preview)
      : sd2::subproject(in_subproject), preview_file_(in_preview) {}
  // to json
  friend void to_json(nlohmann::json& j, const seedance2_subproject_and_person& p) {
    to_json(j, static_cast<const sd2::subproject&>(p));
    j["persons"]      = p.persons_;
    j["preview_file"] = p.preview_file_;
  }
};
}  // namespace

// /api/seedance2/subproject
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject, get) {
  auto l_sql = get_sqlite_database();
  using namespace orm;

  auto l_query = select(l_sql)
                     .columns(object<sd2::subproject>(), object<sd2::ai_preview_file>())
                     .from<sd2::subproject>()
                     .left_outer_join<sd2::ai_preview_file>(&sd2::subproject::preview_file_, &sd2::ai_preview_file::uuid_id_);

  // 非制片/管理员只能看到自己参与的子项目
  if (!person_.is_manager()) {
    l_query.join<sd2::subproject_person_link>(&sd2::subproject_person_link::subproject_id_, &sd2::subproject::uuid_id_);
    l_query.where(c(&sd2::subproject_person_link::person_id_) == person_.person_.uuid_id_);
  }

  auto l_subprojects = l_query().to_vector();

  std::vector<uuid> l_subproject_ids;
  l_subproject_ids.reserve(l_subprojects.size());
  for (auto&& [l_sp, l_] : l_subprojects) l_subproject_ids.push_back(l_sp.uuid_id_);

  auto l_all_links = select(l_sql)
                         .columns(object<sd2::subproject_person_link>())
                         .from<sd2::subproject_person_link>()
                         .where(c(&sd2::subproject_person_link::subproject_id_).in(l_subproject_ids))()
                         .to_vector();
  std::map<uuid, std::vector<uuid>> l_person_map;
  for (auto&& l_link : l_all_links) {
    l_person_map[l_link.subproject_id_].push_back(l_link.person_id_);
  }

  std::vector<seedance2_subproject_and_person> l_result;
  l_result.reserve(l_subprojects.size());
  for (auto&& [l_sp, l_preview] : l_subprojects) {
    auto& l_item                          = l_result.emplace_back(std::move(l_sp), std::move(l_preview));
    if (l_person_map.contains(l_item.uuid_id_)) l_item.persons_ = std::move(l_person_map[l_item.uuid_id_]);
  }

  co_return in_handle->make_msg(nlohmann::json{} = l_result);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject, post) {
  person_.check_manager();
  auto l_sql        = get_sqlite_database();
  auto l_json       = in_handle->get_json();

  auto l_subproject = std::make_shared<sd2::subproject>();
  l_json.get_to(*l_subproject);
  l_subproject->created_user_id_ = person_.person_.uuid_id_;

  co_await l_sql.install(l_subproject);

  co_return in_handle->make_msg(nlohmann::json{} = *l_subproject);
}

// /api/seedance2/subproject/{id}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_instance, get) {
  person_.check_manager();
  auto l_sql        = get_sqlite_database();
  auto l_subproject = l_sql.get_by_uuid<sd2::subproject>(id_);

  co_return in_handle->make_msg(nlohmann::json{} = l_subproject);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_instance, put) {
  person_.check_manager();
  auto l_sql        = get_sqlite_database();
  auto l_json       = in_handle->get_json();

  auto l_subproject = std::make_shared<sd2::subproject>(l_sql.get_by_uuid<sd2::subproject>(id_));
  if (l_json.contains("name")) l_json.at("name").get_to(l_subproject->name_);
  if (l_json.contains("project_id")) l_json.at("project_id").get_to(l_subproject->project_id_);

  co_await l_sql.update(l_subproject);

  co_return in_handle->make_msg(nlohmann::json{} = *l_subproject);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_instance, delete_) {
  person_.check_manager();
  auto l_sql   = get_sqlite_database();

  bool l_force = false;
  for (auto&& l_param : in_handle->url_.params()) {
    if (l_param.key == "force" && l_param.has_value && l_param.value == "true") {
      l_force = true;
      break;
    }
  }

  if (l_force) {
    person_.check_admin();
    co_await l_sql.remove<sd2::subproject>(id_);
  } else {
    auto l_subproject       = std::make_shared<sd2::subproject>(l_sql.get_by_uuid<sd2::subproject>(id_));
    l_subproject->archived_ = true;
    co_await l_sql.update(l_subproject);
  }

  co_return in_handle->make_msg_204();
}

// /api/seedance2/subproject/{id}/preview
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_preview, post) {
  auto l_sql            = get_sqlite_database();
  auto l_subproject     = std::make_shared<sd2::subproject>(l_sql.get_by_uuid<sd2::subproject>(id_));
  auto l_file           = in_handle->get_file();
  auto l_preview        = std::make_shared<sd2::ai_preview_file>();
  l_preview->extension_ = ".png";
  co_await l_sql.install(l_preview);
  l_subproject->preview_file_ = l_preview->uuid_id_;
  co_await l_sql.update(l_subproject);

  auto& l_ctx           = g_ctx().get<kitsu_ctx_t>();
  auto l_file_picture   = l_ctx.get_sd2_pictures_file(l_preview->uuid_id_, l_file.extension().string());
  auto l_file_thumbnail = l_ctx.get_sd2_thumbnail_file(l_preview->uuid_id_);

  if (auto l_p = l_file_picture.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
  if (auto l_p = l_file_thumbnail.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);

  {
    auto l_image = cv::imread(l_file.generic_string());
    if (l_image.empty()) throw_exception(doodle_error{"图片解码失败"});
    auto l_resize = std::min(500.0 / l_image.cols, 500.0 / l_image.rows);
    cv::resize(l_image, l_image, cv::Size(l_image.cols * l_resize, l_image.rows * l_resize));
    cv::imwrite(l_file_thumbnail.generic_string(), l_image);
  }
  FSys::rename(l_file, l_file_picture);

  co_return in_handle->make_msg(nlohmann::json{{"id", l_preview->uuid_id_}});
}

namespace {

std::optional<sd2::subproject_person_link> get_subproject_person_link(
    const uuid& in_subproject_id, const uuid& in_person_id
) {
  auto l_sql = get_sqlite_database();
  using namespace orm;
  return select(l_sql)
      .columns(object<sd2::subproject_person_link>())
      .from<sd2::subproject_person_link>()
      .where(c(&sd2::subproject_person_link::subproject_id_) == in_subproject_id && c(&sd2::subproject_person_link::person_id_) == in_person_id)()
      .to_optional();
}

}  // namespace

// /api/seedance2/subproject/{subproject_id}/person
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_person_link, post) {
  person_.check_manager();
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  auto l_link = std::make_shared<sd2::subproject_person_link>();
  l_json.get_to(*l_link);
  l_link->subproject_id_ = subproject_id_;

  co_await l_sql.install(l_link);

  co_return in_handle->make_msg(nlohmann::json{} = *l_link);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_person_link, delete_) {
  person_.check_manager();
  auto l_json      = in_handle->get_json();
  auto l_person_id = l_json.at("person_id").get<uuid>();

  auto l_link      = get_subproject_person_link(subproject_id_, l_person_id);
  if (!l_link)
    co_return in_handle->make_msg(nlohmann::json{{"subproject_id", subproject_id_}, {"person_id", l_person_id}});

  auto l_sql = get_sqlite_database();
  co_await l_sql.remove<sd2::subproject_person_link>(l_link->uuid_id_);

  co_return in_handle->make_msg(nlohmann::json{{"subproject_id", subproject_id_}, {"person_id", l_person_id}});
}

}  // namespace doodle::http::seedance2