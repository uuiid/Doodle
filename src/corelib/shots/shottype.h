/*
 * @Author: your name
 * @Date: 2020-09-15 14:21:02
 * @LastEditTime: 2020-12-14 16:03:12
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\shots\shottype.h
 */
#pragma once

#include <corelib/core_global.h>
#include <corelib/core/CoreData.h>

#include <boost/signals2.hpp>
DOODLE_NAMESPACE_S

class CORE_API shotType : public CoreData,
                          public std::enable_shared_from_this<shotType> {
  RTTR_ENABLE(CoreData)
 public:
  shotType();
  ~shotType();

  //根据fileclass外键查询filetype
  static shotTypePtrList getAll();

  bool setInfo(const std::string &value) override;

  void setType(const dstring &value);
  //获得本身的字符串属性
  dstring getType() const;
  //获得外键连接的实体对象 shotclass
  shotClassPtr getFileClass();

  //根据字符串寻找tyep
  static shotTypePtr findShotType(const std::string &type_name);
  static shotTypePtr findShotType(const std::string &type_nmae,
                                  bool autoInstall);
  static const std::unordered_set<shotType *> Instances();

  DOODLE_DISABLE_COPY(shotType);

  static boost::signals2::signal<void(const shotTypePtr &)> insertChanged;

 private:
  //设置和连接外键 shotclass
  void setShotClass(const shotClassPtr &fileclass_);

 private:
  //自身属性
  dstring p_Str_Type;

  //指针属性
  shotClassPtr p_class_ptr_;

  //指针id属性
  int64_t p_shotClass_id;
  DOODLE_INSRANCE(shotType);
};

DOODLE_NAMESPACE_E
