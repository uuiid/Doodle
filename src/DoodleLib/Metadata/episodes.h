#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Metadata/metadata.h>

namespace doodle {

class DOODLELIB_API episodes : public metadata {
  int64_t p_episodes;

 public:
  episodes();
  explicit episodes(std::weak_ptr<metadata> in_metadata, int64_t in_episodes);
  // ~Episodes();

  [[nodiscard]] const int64_t &get_episodes() const noexcept;
  void set_episodes(const int64_t &Episodes_);

  [[nodiscard]] std::string str() const override;

  void attribute_widget(const attribute_factory_ptr &in_factoryPtr) override;

  bool operator<(const episodes &in_rhs) const;
  bool operator>(const episodes &in_rhs) const;
  bool operator<=(const episodes &in_rhs) const;
  bool operator>=(const episodes &in_rhs) const;

  inline bool analysis(const FSys::path &in_path) {
    return analysis(in_path.generic_string());
  };
  bool analysis(const std::string &in_path);

  static episodes_ptr analysis_static(const std::string &in_path);
  inline static episodes_ptr analysis_static(const FSys::path &in_path) {
    return analysis_static(in_path.generic_string());
  };

 private:
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version);
};
template <class Archive>
void episodes::serialize(Archive &ar, const std::uint32_t version) {
  if (version == 1)
    ar &boost::serialization::make_nvp("Metadata", boost::serialization::base_object<metadata>(*this)) &
        p_episodes;
}
}  // namespace doodle

BOOST_CLASS_VERSION(doodle::episodes, 1)
BOOST_CLASS_EXPORT_KEY(doodle::episodes)
