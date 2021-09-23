//
// Created by TD on 2021/5/7.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

namespace doodle {
class user {
 public:
  using time_pair_list = std::vector<std::pair<chrono::sys_time_pos, chrono::sys_time_pos> >;

 private:
  std::string p_string_;
  std::string p_ENUS;

  time_pair_list p_time_rest;
  time_pair_list p_time_work;

 public:
  user();
  explicit user(std::string in_string);
  explicit user(std::string in_string, std::string in_ENUS);

  [[nodiscard]] const std::string& get_name() const;
  void set_name(const std::string& in_string);
  void set_name(const std::string& in_string, const std::string& in_ENUS);

  [[nodiscard]] const std::string& get_enus() const;
  void set_enus(const std::string& in_string);

  const time_pair_list& get_time_work_list() const;
  time_pair_list& get_time_work_list();

  const time_pair_list& get_time_rest_list() const;
  time_pair_list& get_time_rest_list();

 private:
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive& ar, std::uint32_t const version);
};
template <class Archive>
void user::serialize(Archive& ar, const std::uint32_t version) {
  if (version == 1)
    ar& p_string_&
        p_ENUS&
            p_time_rest&
                p_time_work;
}

}  // namespace doodle
BOOST_CLASS_VERSION(doodle::user, 1)
BOOST_CLASS_EXPORT_KEY(doodle::user)
