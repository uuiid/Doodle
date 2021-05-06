//
// Created by teXiao on 2021/4/27.
//

#pragma once

#include <corelib/core_global.h>
#include <corelib/Metadata/Metadata.h>


namespace doodle {
class CORE_API Assets : public Metadata{
  std::string p_name;

 public:
  explicit Assets(std::string in_name);
  std::string str() const override;
};
}