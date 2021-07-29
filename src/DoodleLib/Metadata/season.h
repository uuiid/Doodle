//
// Created by TD on 2021/7/29.
//

#pragma once
#include <DoodleLib/Metadata/Metadata.h>
#include <DoodleLib_fwd.h>
namespace doodle {

class DOODLELIB_API season
    : public Metadata{
  std::int32_t p_int;
 public:
  season();

  explicit season(std::weak_ptr<Metadata> in_metadata,std::int32_t in_);
  virtual std::string str() const override;
  virtual void create_menu(const menu_factory_ptr & in_factoryPtr) override;
};
}  // namespace doodle
