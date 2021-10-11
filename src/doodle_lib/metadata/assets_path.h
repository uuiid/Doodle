//
// Created by TD on 2021/5/18.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/metadata/leaf_meta.h>
#include <doodle_lib/metadata/tree_adapter.h>
namespace doodle {
class DOODLELIB_API assets_path : public leaf_meta {
  /**
   * @brief 上传时的本地路径
   *
   */
  FSys::path p_local_path;
  /**
   * @brief 上传时相对本地根的路径
   *
   */
  FSys::path p_lexically_relative;
  /**
   * @brief 服务器路径
   *
   */
  FSys::path p_server_path;
  /**
   * @brief 服务器中如果存在文件的备份路径
   *
   */
  FSys::path p_backup_path;

 public:
  assets_path();
  /**
   * @brief 生成一个类
   *
   * @param in_path 输入路径，这个会调用 AssetsPath::set_path(const FSys::path &in_path, const MetadataConstPtr &in_metadata)
   */
  explicit assets_path(const FSys::path &in_path, const metadata_const_ptr &in_metadata);

  [[nodiscard]] const FSys::path &get_local_path() const;
  [[nodiscard]] FSys::path get_cache_path() const;
  [[nodiscard]] const FSys::path &get_server_path() const;
  [[nodiscard]] const FSys::path &get_backup_path() const;
  /**
   * @brief 设置资产的本地文件的路径，根据元数据产生路径
   *
   * 会产生根据元数据标签产生的路径
   *
   * @param in_path  本地文件的路径
   * @param in_metadata 元数据指针，
   */
  void set_path(const FSys::path &in_path, const metadata_const_ptr &in_metadata, bool in_using_lexically_relative = false);

  /**
   * @brief设置资产的本地文件的路径，直接设置服务器路径
   *
   * @param in_local_path 本地路径
   * @param in_server_path 服务器路径
   */
  void set_path(const FSys::path &in_local_path, const FSys::path &in_server_path);

  std::string str() const;
  //  void open();
 private:
  //这里是序列化的代码
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version) {
    if (version == 3)
      ar &boost::serialization::make_nvp("local_path", p_local_path) &
          boost::serialization::make_nvp("lexically_relative", p_lexically_relative) &
          boost::serialization::make_nvp("server_path", p_server_path) &
          boost::serialization::make_nvp("backup_path", p_backup_path);
  };
};

class DOODLELIB_API assets_path_vector
    : public details::no_copy,
      public leaf_meta {
 public:
  assets_path_vector() : paths(){};
  using path_list = std::vector<assets_path_ptr>;
  path_list paths;

  inline vector_adapter<path_list, assets_path_vector> get() {
    return make_vector_adapter(paths, *this);
  };

  void set_metadata(const std::weak_ptr<metadata> &in_meta) override;

  inline void end_push_back(const assets_path_ptr &in) {
    if (!p_meta.expired())
      in->set_metadata(p_meta);
    //p_meta.lock()->saved(true);
  };
  inline void end_clear(){};

  path_list add_file(const FSys::path &in_path, bool in_using_lexically_relative = false);

 private:
  //这里是序列化的代码
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version) {
    if (version == 1)
      ar &boost::serialization::make_nvp("paths", paths);
  };
};

}  // namespace doodle

namespace fmt {

template <>
struct fmt::formatter<doodle::assets_path> : fmt::formatter<fmt::string_view> {
  template <typename FormatContext>
  auto format(const doodle::assets_path &in_, FormatContext &ctx) {
    return formatter<string_view>::format(
        in_.get_server_path().generic_string(),
        ctx);
  }
};
template <>
struct fmt::formatter<doodle::assets_path_vector> : fmt::formatter<fmt::string_view> {
  template <typename FormatContext>
  auto format(const doodle::assets_path_vector &in_, FormatContext &ctx) -> decltype(ctx.out()) {
    std::string str;
    for (auto &i : in_.paths) {
      str += fmt::format("{}\n", i->get_server_path().generic_string());
    }

    return formatter<string_view>::format(
        str,
        ctx);
  }
};

}  // namespace fmt

BOOST_CLASS_VERSION(doodle::assets_path, 3)
BOOST_CLASS_EXPORT_KEY(doodle::assets_path)
BOOST_CLASS_VERSION(doodle::assets_path_vector, 1)
BOOST_CLASS_EXPORT_KEY(doodle::assets_path_vector)
