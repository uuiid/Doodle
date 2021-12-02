//
// Created by TD on 2021/5/18.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/metadata/leaf_meta.h>
#include <doodle_lib/metadata/tree_adapter.h>

namespace doodle {

class DOODLELIB_API comment {
  std::string p_comment;
  std::string p_user;

 public:
  comment();
  explicit comment(std::string in_str);
  [[nodiscard]] const std::string& get_comment() const;
  void set_comment(const std::string& in_comment);
  [[nodiscard]] const std::string& get_user() const;
  void set_user(const std::string& in_user);
  DOODLE_MOVE(comment);

 private:
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive& ar, const std::uint32_t version) {
    if (version == 1)
      ar& BOOST_SERIALIZATION_NVP(p_comment) &
          BOOST_SERIALIZATION_NVP(p_user);
  };

  friend void to_json(nlohmann::json& j, const comment& p) {
    j["comment"] = p.p_comment;
    j["user"]    = p.p_user;
  }
  friend void from_json(const nlohmann::json& j, comment& p) {
    j.at("comment").get_to(p.p_comment);
    j.at("user").get_to(p.p_user);
  }
};

class DOODLELIB_API comment_vector {
 public:
  comment_vector() : comm(){};

  std::vector<comment> comm;

  void end_push_back(const comment& in){
      // p_meta.lock()->saved(true);
  };

  void end_erase(const comment& in){};
  void end_clear(){};

  inline vector_adapter<std::vector<comment>, comment_vector> get() {
    return make_vector_adapter(comm, *this);
  };
  // inline const vector_adapter<comment, comment_vector> get() const {
  //   return vector_adapter<comment, comment_vector>{comm, *this};
  // };
  DOODLE_MOVE(comment_vector);

 private:
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive& ar, const std::uint32_t version) {
    if (version == 1)
      ar& BOOST_SERIALIZATION_NVP(comm);
  };
  friend void to_json(nlohmann::json& j, const comment_vector& p) {
    j["vector"] = p.comm;
  }
  friend void from_json(const nlohmann::json& j, comment_vector& p) {
    j.at("vector").get_to(p.comm);
  }
};
}  // namespace doodle

namespace fmt {
template <>
struct formatter<doodle::comment> : formatter<string_view> {
  template <typename FormatContext>
  auto format(const doodle::comment& in_, FormatContext& ctx) {
    formatter<string_view>::format(
        in_.get_comment(),
        ctx);
  }
};
}  // namespace fmt
BOOST_CLASS_VERSION(doodle::comment, 1)
BOOST_CLASS_EXPORT_KEY(doodle::comment)
BOOST_CLASS_VERSION(doodle::comment_vector, 1)
BOOST_CLASS_EXPORT_KEY(doodle::comment_vector)
