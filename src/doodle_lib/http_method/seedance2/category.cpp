#include "doodle_core/doodle_core_fwd.h"
#include <doodle_core/metadata/seedance2/ai_category.h>
#include <doodle_core/metadata/seedance2/ai_generate_entity.h>
#include <doodle_core/metadata/seedance2/ai_preview_file.h>

#include <doodle_lib/http_method/seedance2/reg.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include "core/global_function.h"
#include "reg.h"
#include <memory>

namespace doodle::http::seedance2 {
namespace sd2 = doodle::seedance2;

// /api/seedance2/subproject/{subproject_id}/category
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_ai_category, get) {
  person_.check_subproject_access(subproject_id_);

  auto l_sql = get_sqlite_database();
  using namespace orm;
  auto l_result = select(l_sql)
                      .columns(object<sd2::ai_category>())
                      .from<sd2::ai_category>()()
                      .to_vector();
  co_return in_handle->make_msg(nlohmann::json{} = l_result);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_ai_category, post) {
  person_.check_manager();
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  auto l_category = std::make_shared<sd2::ai_category>();
  l_json.get_to(*l_category);

  co_await l_sql.install(l_category);

  co_return in_handle->make_msg(nlohmann::json{} = *l_category);
}

// /api/seedance2/subproject/{subproject_id}/category/{category_id}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_ai_category_instance, get) {
  person_.check_subproject_access(subproject_id_);

  auto l_sql      = get_sqlite_database();
  auto l_category = l_sql.get_by_uuid<sd2::ai_category>(category_id_);

  co_return in_handle->make_msg(nlohmann::json{} = l_category);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_ai_category_instance, put) {
  person_.check_manager();
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  auto l_category = std::make_shared<sd2::ai_category>(l_sql.get_by_uuid<sd2::ai_category>(category_id_));
  if (l_json.contains("name")) l_json.at("name").get_to(l_category->name_);
  if (l_json.contains("description")) l_json.at("description").get_to(l_category->description_);
  if (l_json.contains("type")) l_json.at("type").get_to(l_category->type_);

  co_await l_sql.update(l_category);

  co_return in_handle->make_msg(nlohmann::json{} = *l_category);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_ai_category_instance, delete_) {
  person_.check_manager();
  person_.check_not_outsourcer();
  auto l_sql = get_sqlite_database();
  using namespace orm;

  // 删除类别时, 将该类别下所有实体的 ai_category_id 置空
  co_await l_sql.update(
      orm::update(l_sql)
          .from<sd2::ai_generate_entity>()
          .set(c(&sd2::ai_generate_entity::ai_category_id_) = uuid{})
          .where(c(&sd2::ai_generate_entity::ai_category_id_) == category_id_)
  );
  co_await l_sql.remove<sd2::ai_category>(category_id_);

  co_return in_handle->make_msg(nlohmann::json{{"category_id", category_id_}});
}

// /api/seedance2/subproject/{subproject_id}/category/{category_id}/entity
namespace {
struct category_entity_with_preview_file : public sd2::ai_generate_entity {
  sd2::ai_preview_file preview_;
  explicit category_entity_with_preview_file(
      const sd2::ai_generate_entity& in_entity, const sd2::ai_preview_file& in_preview
  )
      : sd2::ai_generate_entity(in_entity), preview_(in_preview) {}
  // to  json
  friend void to_json(nlohmann::json& j, const category_entity_with_preview_file& p) {
    to_json(j, static_cast<const sd2::ai_generate_entity&>(p));
    if (!p.preview_.uuid_id_.is_nil()) j["preview"] = p.preview_;
  }
};
}  // namespace

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_ai_category_entity, get) {
  person_.check_subproject_access(subproject_id_);

  auto l_sql = get_sqlite_database();
  using namespace orm;
  auto l_result = select(l_sql)
                      .columns(object<sd2::ai_generate_entity>(), object<sd2::ai_preview_file>())
                      .from<sd2::ai_generate_entity>()
                      .left_outer_join<sd2::ai_preview_file>(
                          &sd2::ai_generate_entity::preview_file_, &sd2::ai_preview_file::uuid_id_
                      )
                      .where(c(&sd2::ai_generate_entity::ai_category_id_) == category_id_)()
                      .to_vector<category_entity_with_preview_file>();
  co_return in_handle->make_msg(nlohmann::json{} = l_result);
}

}  // namespace doodle::http::seedance2
