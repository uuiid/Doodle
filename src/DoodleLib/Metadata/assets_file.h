﻿//
// Created by TD on 2021/5/7.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Metadata/metadata.h>
#include <DoodleLib/core/core_set.h>

namespace doodle {
/**
 * @brief 这个类代表着服务端的文件条目
 *
 */
class DOODLELIB_API assets_file : public metadata {
  std::string p_name;
  std::string p_ShowName;
  AssetsPathPtr p_path_file;
  std::vector<AssetsPathPtr> p_path_files;
  TimeDurationPtr p_time;
  std::string p_user;
  department p_department;
  std::vector<CommentPtr> p_comment;
  std::uint64_t p_version;

  bool p_need_time;

 public:
  /**
   * @brief 默认构造
   *
   */
  assets_file();

  /**
   * @brief 构造一个条目并添加一些必要的属性
   *
   * @param in_metadata 父条目,这个是必须的
   * @param in_path 本地路径,这个在创建时会自动生成一个服务器路径(基本上是一个uuid路径)
   *                基本是调用 AssetsPath::set_path(const FSys::path &in_path, const MetadataConstPtr &in_metadata)
   * @param name 名称
   * @param showName 显示名称
   */
  explicit assets_file(std::weak_ptr<metadata> in_metadata, std::string showName, std::string Name = {});
  // ~AssetsFile();

  [[nodiscard]] std::string str() const override;
  [[nodiscard]] std::string show_str() const override;

  [[nodiscard]] const TimeDurationPtr& get_time() const;
  void set_time(const TimeDurationPtr& in_time);

  [[nodiscard]] const std::string& get_user() const;
  void set_user(const std::string& in_user);
  const std::vector<AssetsPathPtr>& get_path_file() const;
  std::vector<AssetsPathPtr>& get_path_file();
  void set_path_file(const std::vector<AssetsPathPtr>& in_pathFile);
  department get_department() const;
  void set_department(department in_department);

  [[nodiscard]] const std::vector<CommentPtr>& get_comment() const;
  void set_comment(const std::vector<CommentPtr>& in_comment);
  void add_comment(const CommentPtr& in_comment);

  const std::uint64_t& get_version() const noexcept;
  std::string get_version_str() const;
  void set_version(const std::uint64_t& in_Version) noexcept;
  int find_max_version() const;

  virtual void attribute_widget(const attribute_factory_ptr& in_factoryPtr) override;

  bool operator<(const assets_file& in_rhs) const;
  bool operator>(const assets_file& in_rhs) const;
  bool operator<=(const assets_file& in_rhs) const;
  bool operator>=(const assets_file& in_rhs) const;

  virtual void to_DataDb(DataDb& in_) const override;

 private:
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive& ar, std::uint32_t const version);
};

template <class Archive>
void assets_file::serialize(Archive& ar, const std::uint32_t version) {
  if (version == 1)
    ar&
            boost::serialization::make_nvp("Metadata", boost::serialization::base_object<metadata>(*this)) &
        BOOST_SERIALIZATION_NVP(p_name) &
        BOOST_SERIALIZATION_NVP(p_ShowName) &
        BOOST_SERIALIZATION_NVP(p_path_file) &
        BOOST_SERIALIZATION_NVP(p_time) &
        BOOST_SERIALIZATION_NVP(p_user) &
        BOOST_SERIALIZATION_NVP(p_department) &
        BOOST_SERIALIZATION_NVP(p_comment);
  if (version == 2)
    ar&
            boost::serialization::make_nvp("Metadata", boost::serialization::base_object<metadata>(*this)) &
        BOOST_SERIALIZATION_NVP(p_name) &
        BOOST_SERIALIZATION_NVP(p_ShowName) &
        BOOST_SERIALIZATION_NVP(p_path_file) &
        BOOST_SERIALIZATION_NVP(p_time) &
        BOOST_SERIALIZATION_NVP(p_user) &
        BOOST_SERIALIZATION_NVP(p_department) &
        BOOST_SERIALIZATION_NVP(p_comment) &
        BOOST_SERIALIZATION_NVP(p_version);
  if (version == 3)
    ar&
            boost::serialization::make_nvp("Metadata", boost::serialization::base_object<metadata>(*this)) &
        BOOST_SERIALIZATION_NVP(p_name) &
        BOOST_SERIALIZATION_NVP(p_ShowName) &
        BOOST_SERIALIZATION_NVP(p_path_files) &
        BOOST_SERIALIZATION_NVP(p_time) &
        BOOST_SERIALIZATION_NVP(p_user) &
        BOOST_SERIALIZATION_NVP(p_department) &
        BOOST_SERIALIZATION_NVP(p_comment) &
        BOOST_SERIALIZATION_NVP(p_version);
}

}  // namespace doodle

BOOST_CLASS_VERSION(doodle::assets_file, 3)
BOOST_CLASS_EXPORT_KEY(doodle::assets_file)
