#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Metadata/Metadata.h>

namespace doodle {

class DOODLELIB_API Episodes : public Metadata {
  int64_t p_episodes;

 public:
  Episodes();
  explicit Episodes(std::weak_ptr<Metadata> in_metadata, int64_t in_episodes);
  // ~Episodes();

  [[nodiscard]] const int64_t &getEpisodes() const noexcept;
  void setEpisodes(const int64_t &Episodes_);

  [[nodiscard]] std::string str() const override;

  void create_menu(const attribute_factory_ptr &in_factoryPtr) override;

  bool operator<(const Episodes &in_rhs) const;
  bool operator>(const Episodes &in_rhs) const;
  bool operator<=(const Episodes &in_rhs) const;
  bool operator>=(const Episodes &in_rhs) const;

  inline bool analysis(const FSys::path &in_path) {
    return analysis(in_path.generic_string());
  };
  bool analysis(const std::string &in_path);

  static EpisodesPtr analysis_static(const std::string &in_path);
  inline static EpisodesPtr analysis_static(const FSys::path &in_path) {
    return analysis_static(in_path.generic_string());
  };

 private:
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version);
};
template <class Archive>
void Episodes::serialize(Archive &ar, const std::uint32_t version) {
  if (version == 1)
    ar &boost::serialization::make_nvp("Metadata", boost::serialization::base_object<Metadata>(*this)) &
        p_episode;
}
}  // namespace doodle

BOOST_CLASS_VERSION(doodle::Episodes, 1)
BOOST_CLASS_EXPORT_KEY(doodle::Episodes)
