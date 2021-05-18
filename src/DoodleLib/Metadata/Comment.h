//
// Created by TD on 2021/5/18.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
namespace doodle{

class DOODLELIB_API Comment {
 std::string p_comment;
 std::string p_user;

 public:
  Comment();
  explicit Comment(std::string in_str);
  [[nodiscard]] const std::string& GetComment() const;
  void SetComment(const std::string& in_comment);
  [[nodiscard]] const std::string& GetUser() const;
  void SetUser(const std::string& in_user);
};

}
