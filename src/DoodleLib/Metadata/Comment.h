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
  [[nodiscard]] const std::string& getComment() const;
  void setComment(const std::string& in_comment);
  [[nodiscard]] const std::string& getUser() const;
  void setUser(const std::string& in_user);
};

}
