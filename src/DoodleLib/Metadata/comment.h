//
// Created by TD on 2021/5/18.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Metadata/leaf_meta.h>
#include <DoodleLib/Metadata/tree_adapter.h>

namespace doodle {

class DOODLELIB_API comment : public leaf_meta {
  std::string p_comment;
  std::string p_user;

 public:
  comment();
  explicit comment(std::string in_str);
  [[nodiscard]] const std::string& get_comment() const;
  void set_comment(const std::string& in_comment);
  [[nodiscard]] const std::string& get_user() const;
  void set_user(const std::string& in_user);

 private:
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive& ar, const std::uint32_t version) {
    if (version == 1)
      ar& BOOST_SERIALIZATION_NVP(p_comment) &
          BOOST_SERIALIZATION_NVP(p_user);
  };
};

class DOODLELIB_API comment_vector
    : public details::no_copy,
      public leaf_meta {
 public:
  comment_vector() : comm(){};

  std::vector<comment_ptr> comm;

  void end_push_back(const comment_ptr& in) {
    in->set_metadata(p_meta.lock());
  };

  void end_erase(const comment_ptr& in){};
  void end_clear(){};

  inline vector_adapter<std::vector<comment_ptr>, comment_vector> get() {
    return make_vector_adapter(comm, *this);
  };
  // inline const vector_adapter<comment_ptr, comment_vector> get() const {
  //   return vector_adapter<comment_ptr, comment_vector>{comm, *this};
  // };

  void set_metadata(const metadata_ptr& in_meta) override;

 private:
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive& ar, const std::uint32_t version) {
    if (version == 1)
      ar& BOOST_SERIALIZATION_NVP(comm);
  };
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
