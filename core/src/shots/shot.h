/*
 * @Author: your name
 * @Date: 2020-09-15 13:57:51
 * @LastEditTime: 2020-12-14 15:56:58
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\shot.h
 */
#pragma once

#include <core_global.h>
#include <src/core/coresqldata.h>

CORE_NAMESPACE_S

class CORE_API shot : public coresqldata,
                      public std::enable_shared_from_this<shot> {
 public:
  enum class e_shotAB {
    _ = 0,
    B = 1,
    C = 2,
    D = 3,
    E = 4,
    F = 5,
    G = 6,
    H = 7
  };
  const static std::vector<std::string> e_shotAB_list;
  shot();
  ~shot();
  //使用id直接从数据库创建类
  void select(const qint64 &ID_);

  //数据库语句发出
  void insert() override;
  void updateSQL() override;
  void deleteSQL() override;

  //使用episodes 的外键约束创建多个类
  static shotPtrList getAll(const episodesPtr &EP_);

  //设置episodes约束外键
  void setEpisodes(const episodesPtr &value);
  //获得episodes 约束实体
  episodesPtr getEpisodes();

  //设置shot自身信息
  void setShot(const int64_t &sh, const e_shotAB &ab = e_shotAB::_);
  void setShot(const int64_t &sh, const dstring &ab);
  void setShot(const int64_t &sh, const QString &ab);
  void setShotAb(const dstring &ab);
  void setShotAb(const e_shotAB &ab) { p_qenm_shotab = ab; };
  //获得全部的shot的str格式化信息
  dstring getShotAndAb_str() const;
  QString getShotAndAb_strQ() const;
  //只有shot的格式化信息
  dstring getShot_str() const;
  //只有shot Ab的格式化信息
  dstring getShotAb_str() const;
  QString getShotAb_strQ() const;
  //获得shot的数值
  int64_t getShot() const { return p_qint_shot_; };

  static const std::map<int64_t, shot *> &Instances();

 private:
  int64_t p_qint_shot_;
  e_shotAB p_qenm_shotab;

  episodesPtr p_ptr_eps;
  int64_t p_eps_id;
  DOODLE_INSRANCE(shot);
  RTTR_ENABLE(coresqldata)
};
inline void shot::setShot(const int64_t &sh, const QString &ab) {
  setShot(sh, ab.toStdString());
}
inline QString shot::getShotAndAb_strQ() const {
  return QString::fromStdString(getShotAndAb_str());
}
inline QString shot::getShotAb_strQ() const {
  return QString::fromStdString(getShotAb_str());
}
CORE_NAMESPACE_E
