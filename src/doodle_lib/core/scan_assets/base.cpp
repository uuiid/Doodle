//
// Created by TD on 2023/12/20.
//
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/main_map.h>

#include <core/scan_assets/base.h>
#include <cryptopp/base64.h>
#include <cryptopp/files.h>
#include <cryptopp/hex.h>
#include <cryptopp/sha.h>
namespace doodle::details {

scan_category_data_t::operator scan_data_t::database_t() const {
  return scan_data_t::database_t{
      .ue_uuid_    = ue_file_.uuid_.is_nil() ? std::nullopt : std::optional{ue_file_.uuid_},
      .rig_uuid_   = rig_file_.uuid_.is_nil() ? std::nullopt : std::optional{rig_file_.uuid_},
      .solve_uuid_ = solve_file_.uuid_.is_nil() ? std::nullopt : std::optional{solve_file_.uuid_},
      .project_id_ = project_database_ptr->id_,
      .ue_path_    = ue_file_.uuid_.is_nil() ? std::nullopt : std::optional{ue_file_.path_},
      .rig_path_   = rig_file_.uuid_.is_nil() ? std::nullopt : std::optional{rig_file_.path_},
      .solve_path_ = solve_file_.uuid_.is_nil() ? std::nullopt : std::optional{solve_file_.path_},
      .season_     = season_.p_int,
      .dep_        = assets_type_,
      .name_       = name_,
      .version_    = version_name_.empty() ? std::nullopt : std::optional{version_name_},
      .num_        = number_str_.empty() ? std::nullopt : std::optional{number_str_}
  };
}

std::string scan_category_t::file_hash(const std::string& in_data) {
  CryptoPP::SHA224 k_sha_224;
  std::string l_str{};
  CryptoPP::StringSource k_file{
      in_data, true, new CryptoPP::HashFilter{k_sha_224, new CryptoPP::HexEncoder{new CryptoPP::StringSink{l_str}}}
  };
  return l_str;
}

void scan_category_t::scan_file_hash(const scan_category_data_ptr& in_data) {
  std::string l_hase_data{};
  if (FSys::exists(in_data->ue_file_.path_)) {
    auto l_f = ue_main_map::find_ue_project_file(in_data->ue_file_.path_);
    l_hase_data += fmt::format(
        "{}{}{}", l_f.filename(), chrono::clock_cast<chrono::system_clock>(FSys::last_write_time(l_f)),
        FSys::file_size(l_f)
    );
    for (auto&& l_r : FSys::recursive_directory_iterator{l_f.parent_path() / doodle_config::ue4_config}) {
      l_hase_data += fmt::format(
          "{}{}{}", l_r.path().filename(), chrono::clock_cast<chrono::system_clock>(l_r.last_write_time()),
          l_r.file_size()
      );
    }
    for (auto&& l_r : FSys::recursive_directory_iterator{l_f.parent_path() / doodle_config::ue4_content}) {
      l_hase_data += fmt::format(
          "{}{}{}", l_r.path().filename(), chrono::clock_cast<chrono::system_clock>(l_r.last_write_time()),
          l_r.file_size()
      );
    }
  }
  if (exists(in_data->rig_file_.path_)) {
    l_hase_data += fmt::format(
        "{}{}{}", in_data->rig_file_.path_.filename(),
        chrono::clock_cast<chrono::system_clock>(FSys::last_write_time(in_data->rig_file_.path_)),
        FSys::file_size(in_data->rig_file_.path_)
    );
  }
  if (exists(in_data->solve_file_.path_)) {
    l_hase_data += fmt::format(
        "{}{}{}", in_data->solve_file_.path_.filename(),
        chrono::clock_cast<chrono::system_clock>(FSys::last_write_time(in_data->solve_file_.path_)),
        FSys::file_size(in_data->solve_file_.path_)
    );
  }
  in_data->file_hash_ = file_hash(l_hase_data);
}

}  // namespace doodle::details