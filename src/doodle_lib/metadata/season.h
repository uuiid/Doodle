//
// Created by TD on 2021/7/29.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/metadata/metadata.h>
namespace doodle {

class DOODLELIB_API season {

 public:
  std::int32_t p_int;
  season();
  explicit season( std::int32_t in_);

  void set_season(std::int32_t in_);
  std::int32_t get_season() const;

  virtual std::string str() const ;
  virtual void attribute_widget(const attribute_factory_ptr& in_factoryPtr) ;
  bool operator<(const season& in_rhs) const;
  bool operator>(const season& in_rhs) const;
  bool operator<=(const season& in_rhs) const;
  bool operator>=(const season& in_rhs) const;
  bool operator==(const season& in_rhs) const;
  bool operator!=(const season& in_rhs) const;

 private:

  friend void to_json(nlohmann::json &j, const season &p) {
    j["season"] = p.p_int;
  }
  friend void from_json(const nlohmann::json &j, season &p) {
    j.at("season").get_to(p.p_int);
  }
};

}  // namespace doodle
