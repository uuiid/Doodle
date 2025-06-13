//
// Created by TD on 25-6-13.
//
#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_front_end.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
namespace doodle::http {
namespace {
struct projects_metadata_descriptors_post_arg {
  std::string entity_type_;
  std::string name_;
  metadata_descriptor_data_type data_type_;
  bool for_client_;
  std::vector<std::string> choices_;
  std::vector<uuid> department_;


  operator metadata_descriptor() const {
    metadata_descriptor md;
    md.uuid_id_ = core_set::get_set().get_uuid();
    md.entity_type_ = entity_type_;
    md.name_ = name_;
    md.data_type_ = data_type_;
    md.for_client_ = for_client_;
    md.choices_ = choices_;
    md.department_ = department_;

    return md;
  }

  // from json
  friend void from_json(const nlohmann::json& j, projects_metadata_descriptors_post_arg& p) {
    j.at("entity_type").get_to(p.entity_type_);
    j.at("name").get_to(p.name_);
    j.at("data_type").get_to(p.data_type_);
    j.at("for_client").get_to(p.for_client_);
    j.at("choices").get_to(p.choices_);
    j.at("department").get_to(p.department_);
    static constexpr std::set<std::string_view> g_entity_type = {
        "Asset", "Shot", "Edit", "Episode", "Sequence",
    };
    static constexpr std::set<std::string_view> g_data_type = {
        "string", "number", "list", "taglist", "boolean", "checklist",
    };

    if (!g_entity_type.contains(p.entity_type_))
      throw_exception(http_request_error{boost::beast::http::status::bad_request, "entity_type error"});
    if (p.name_.empty()) throw_exception(http_request_error{boost::beast::http::status::bad_request, "name error"});
  }
};
}  // namespace
// boost::asio::awaitable<boost::beast::http::message_generator> projects_metadata_descriptors_post::callback(
//     session_data_ptr in_handle
// ) {
//   auto l_p   = get_person(in_handle);
//   auto l_arg = in_handle->get_json().get<projects_metadata_descriptors_post_arg>();
// }

}  // namespace doodle::http