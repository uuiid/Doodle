#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Metadata/Metadata.h>

namespace doodle {
class DOODLELIB_API Shot : public Metadata {
  int64_t p_shot;
  std::string p_shot_ab;

  std::weak_ptr<Episodes> p_episodes;

 public:
  Shot();
  Shot(std::weak_ptr<Metadata> in_metadata,
       decltype(p_shot) in_shot,
       decltype(p_shot_ab) in_shot_ab      = {},
       std::weak_ptr<Episodes> in_episodes = {});

  // clang-format off
  enum class ShotAbEnum { A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z };
  // clang-format on

  [[nodiscard]] const decltype(p_shot) &Shot_() const noexcept;
  void setShot_(const decltype(p_shot) &Shot_);

  [[nodiscard]] const decltype(p_shot_ab) &ShotAb() const noexcept;
  void setShotAb(const decltype(p_shot_ab) &ShotAb) noexcept;

  [[nodiscard]] EpisodesPtr Episodes_() const noexcept;
  void setEpisodes_(const EpisodesPtr &Episodes_) noexcept;

  [[nodiscard]] std::string str() const override;

  bool operator<(const Shot &rhs) const;
  bool operator>(const Shot &rhs) const;
  bool operator<=(const Shot &rhs) const;
  bool operator>=(const Shot &rhs) const;

 private:
  friend class cereal::access;
  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version);
};
template <class Archive>
void Shot::serialize(Archive &ar, const std::uint32_t version) {
  if (version == 1)
    ar(
        cereal::make_nvp("Metadata", cereal::base_class<Metadata>(this)),
        p_shot,
        p_shot_ab);
}
}  // namespace doodle

CEREAL_CLASS_VERSION(doodle::Shot, 1)
