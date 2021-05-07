//
// Created by TD on 2021/5/7.
//

#pragma once
namespace doodle {
class Name {
  std::string p_string_;
  std::string p_ENUS;
 public:
  explicit Name(std::string in_string);
  explicit Name(std::string in_string,std::string in_ENUS);

  [[nodiscard]] const std::string& GetName() const;
  void SetName(const std::string& in_string);
  void SetName(const std::string& in_string,const std::string& in_ENUS);

  [[nodiscard]] const std::string& GetENUS() const;
};

}  // namespace doodle
