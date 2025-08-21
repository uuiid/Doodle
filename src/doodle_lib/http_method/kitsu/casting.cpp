//
// Created by TD on 25-8-21.
//
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
#include <doodle_lib/http_method/kitsu/kitsu_result.h>

namespace doodle::http {
struct data_project_sequences_casting_result {
  explicit data_project_sequences_casting_result(
      const entity_link& in_ent_link, const std::string& in_entity_name, const uuid& in_entity_preview_file_id,
      const uuid& in_entity_project_id, const std::string& in_asset_type_name
  )
      : asset_id_(in_ent_link.entity_out_id_),
        name_(in_entity_name),
        asset_name_(in_entity_name),
        asset_type_name_(in_asset_type_name),
        sequence_name_(),
        preview_file_id_(in_entity_preview_file_id),
        nb_occurences_(in_ent_link.nb_occurences_),
        label_(in_ent_link.label_),
        is_shared_(),
        project_id_(in_entity_project_id)

  {}
  uuid asset_id_;
  std::string name_;
  std::string asset_name_;
  std::string asset_type_name_;
  std::string sequence_name_;
  uuid preview_file_id_;
  std::int32_t nb_occurences_;
  std::string label_;
  bool is_shared_;
  uuid project_id_;

  // to json
  friend void to_json(nlohmann::json& j, const data_project_sequences_casting_result& in_data) {
    j["asset_id"]        = in_data.asset_id_;
    j["name"]            = in_data.name_;
    j["asset_name"]      = in_data.asset_name_;
    j["asset_type_name"] = in_data.asset_type_name_;
    j["sequence_name"]   = in_data.sequence_name_;
    j["preview_file_id"] = in_data.preview_file_id_;
    j["nb_occurences"]   = in_data.nb_occurences_;
    j["label"]           = in_data.label_;
    j["is_shared"]       = in_data.is_shared_;
    j["project_id"]      = in_data.project_id_;
  }
};
boost::asio::awaitable<boost::beast::http::message_generator> data_project_sequences_casting::get(
    session_data_ptr in_handle
) {
  person_.check_project_access(project_id_);
  auto l_sql = g_ctx().get<sqlite_database>();
  std::vector<data_project_sequences_casting_result> l_result{};
  {
    using namespace sqlite_orm;
    constexpr auto shot     = "shot"_alias.for_<entity>();
    constexpr auto sequence = "sequence"_alias.for_<entity>();

    for (auto&& [ent_link, entity_name_, entity_preview_file_id_, entity_project_id_, asset_type_name_] :
         l_sql.impl_->storage_any_.select(
             columns(
                 object<entity_link>(true), &entity::name_, &entity::preview_file_id_, &entity::project_id_,
                 &asset_type::name_
             ),
             from<entity_link>(), join<shot>(on(c(&entity_link::entity_in_id_) == c(shot->*&entity::uuid_id_))),
             join<sequence>(on(c(shot->*&entity::parent_id_) == c(sequence->*&entity::uuid_id_))),
             join<entity>(on(c(&entity_link::entity_out_id_) == c(&entity::uuid_id_))),
             join<asset_type>(on(c(&entity::entity_type_id_) == c(&asset_type::uuid_id_))),
             where(c(&entity::canceled_) != true && c(&entity::project_id_) == project_id_)
         )) {
      l_result.emplace_back(
          data_project_sequences_casting_result{
              ent_link, entity_name_, entity_preview_file_id_, entity_project_id_, asset_type_name_
          }
      );
    }
  }
  co_return in_handle->make_msg(nlohmann::json{} = l_result);
}

}  // namespace doodle::http