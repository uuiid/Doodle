#pragma once
#include <corelib/core_global.h>
#include <corelib/Metadata/Metadata.h>

namespace doodle {

class CORE_API Episodes : public Metadata{
  int64_t p_episodes;

 public:
  Episodes();
  explicit Episodes(int64_t in_episodes);

  [[nodiscard]] const int64_t &Episodes_() const noexcept;
  void setEpisodes_(const int64_t &Episodes_);

  [[nodiscard]] std::string str() const override;
};
}  // namespace doodle