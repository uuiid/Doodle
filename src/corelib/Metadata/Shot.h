#pragma once

#include <corelib/core_global.h>
#include <corelib/Metadata/Metadata.h>

namespace doodle {
class CORE_API Shot : public Metadata {
  int64_t p_shot;
  std::string p_shot_ab;

  std::weak_ptr<Episodes> p_episodes;

 public:
  Shot();
  Shot(decltype(p_shot) in_shot,
       decltype(p_shot_ab) in_shot_ab = {},
       decltype(p_episodes) in_episodes = {});

  // clang-format off
  enum class ShotAbEnum { A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z };
  // clang-format on

  [[nodiscard]] const decltype(p_shot) & Shot_() const noexcept;
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
  void serialize(Archive& ar, std::uint32_t const version);
};
template <class Archive>
void Shot::serialize(Archive &ar, const std::uint32_t version) {
  if(version == 1)
    ar(
        cereal::make_nvp("Metadata",cereal::base_class<Metadata>(this)),
        p_shot,
        p_shot_ab
        );
}
}  // namespace doodle
