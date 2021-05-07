//
// Created by teXiao on 2021/4/27.
//

#include <corelib/Metadata/Assets.h>
#include <pinyinlib/convert.h>
namespace doodle {
Assets::Assets()
    : Metadata(),
      p_name() {
}

Assets::Assets(std::string in_name)
    : Metadata(),
      p_name(std::move(in_name)) {}

std::string Assets::str() const {
  return dopinyin::convert::Get().toEn(this->p_name);
}
std::string Assets::ShowStr() const {
  return p_name;
}

}  // namespace doodle
