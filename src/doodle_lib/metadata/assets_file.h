//
// Created by TD on 2021/5/7.
//

#pragma once
#include <doodle_lib/Metadata/metadata.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
/**
 * @brief 这个类代表着服务端的文件条目
 *
 */
class DOODLELIB_API assets_file : public metadata {
  std::string p_name;
  std::string p_ShowName;
  time_wrap_ptr p_time;
  std::string p_user;
  department p_department;
  assets_path_vector_ptr p_path_files;
  comment_vector_ptr p_comment;
  std::uint64_t p_version;

  bool p_need_time;

  void serialize_check();

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

  [[nodiscard]] const time_wrap_ptr& get_time();
  void set_time(const time_wrap_ptr& in_time);

  [[nodiscard]] const std::string& get_user() const;
  void set_user(const std::string& in_user);

  assets_path_vector_ptr get_path_file();
  void set_path_file(const assets_path_vector_ptr& in_);
  [[nodiscard]] comment_vector_ptr get_comment();
  void set_comment(const comment_vector_ptr& in_);

  department get_department() const;
  void set_department(department in_department);

  const std::uint64_t& get_version() const noexcept;
  std::string get_version_str() const;
  void set_version(const std::uint64_t& in_Version) noexcept;

  inline void up_version() noexcept {
    set_version(get_version() + 1);
  };

  int find_max_version() const;

  virtual void attribute_widget(const attribute_factory_ptr& in_factoryPtr) override;

  bool operator<(const assets_file& in_rhs) const;
  bool operator>(const assets_file& in_rhs) const;
  bool operator<=(const assets_file& in_rhs) const;
  bool operator>=(const assets_file& in_rhs) const;

  virtual void to_DataDb(metadata_database& in_) const override;

 private:
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive& ar, std::uint32_t const version);
};

template <class Archive>
void assets_file::serialize(Archive& ar, const std::uint32_t version) {
  if (version == 3)
    ar&
            boost::serialization::make_nvp("metadata", boost::serialization::base_object<metadata>(*this)) &
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
