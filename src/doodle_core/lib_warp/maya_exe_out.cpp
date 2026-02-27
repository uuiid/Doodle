#include "maya_exe_out.h"

namespace doodle {

void from_json(const nlohmann::json& nlohmann_json_j, maya_out_arg& nlohmann_json_t) {
  nlohmann_json_j["begin_time"].get_to(nlohmann_json_t.begin_time);
  nlohmann_json_j["end_time"].get_to(nlohmann_json_t.end_time);
  nlohmann_json_j["out_file_list"].get_to(nlohmann_json_t.out_file_list);
  nlohmann_json_j["movie_file_dir"].get_to(nlohmann_json_t.movie_file_dir);
};

void to_json(nlohmann::json& nlohmann_json_j, const maya_out_arg& nlohmann_json_t) {
  nlohmann_json_j["begin_time"]     = nlohmann_json_t.begin_time;
  nlohmann_json_j["end_time"]       = nlohmann_json_t.end_time;
  nlohmann_json_j["out_file_list"]  = nlohmann_json_t.out_file_list;
  nlohmann_json_j["movie_file_dir"] = nlohmann_json_t.movie_file_dir;
};

}  // namespace doodle