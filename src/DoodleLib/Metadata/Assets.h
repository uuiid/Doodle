//
// Created by teXiao on 2021/4/27.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Metadata/Metadata.h>


namespace doodle {
class DOODLELIB_API Assets : public Metadata{
  std::string p_name;

 public:
  Assets();
  explicit Assets(std::string in_name);



  [[nodiscard]] std::string str() const override;
  [[nodiscard]] std::string ShowStr() const override;
};
}
