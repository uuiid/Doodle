//
// Created by TD on 2021/5/7.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Metadata/Metadata.h>
#include <DoodleLib/core/CoreSet.h>

#include <cereal/types/chrono.hpp>

namespace doodle {
class DOODLELIB_API AssetsFile : public Metadata {
  std::string p_name;
  std::string p_ShowName;
  AssetsPathPtr p_path_file;
  std::chrono::time_point<std::chrono::system_clock> p_time;
  std::string p_user;
  Department p_department;
  std::vector<CommentPtr> p_comment;

 public:
  AssetsFile();
  explicit AssetsFile(std::weak_ptr<Metadata> in_metadata, const FSys::path& in_path, std::string name = {}, std::string showName = {});
  [[nodiscard]] std::string str() const override;
  [[nodiscard]] std::string showStr() const override;

  [[nodiscard]] const std::chrono::time_point<std::chrono::system_clock>& getTime() const;
  void setTime(const std::chrono::time_point<std::chrono::system_clock>& in_time);
  [[nodiscard]] const std::string& getUser() const;
  void setUser(const std::string& in_user);
  const AssetsPathPtr& getPathFile() const;
  void setPathFile(const AssetsPathPtr& in_pathFile);
  Department getDepartment() const;
  void setDepartment(Department in_department);

  [[nodiscard]] const std::vector<CommentPtr>& getComment() const;
  void setComment(const std::vector<CommentPtr>& in_comment);
  void addComment(const CommentPtr& in_comment);

  void select_indb(const MetadataFactoryPtr& in_factory) override;
  void updata_db(const MetadataFactoryPtr& in_factory) override;
  virtual void insert_into(const MetadataFactoryPtr& in_factory) override;
  bool operator<(const AssetsFile& in_rhs) const;
  bool operator>(const AssetsFile& in_rhs) const;
  bool operator<=(const AssetsFile& in_rhs) const;
  bool operator>=(const AssetsFile& in_rhs) const;
  virtual void createMenu(ContextMenu* in_contextMenu) override;
  virtual void deleteData(const MetadataFactoryPtr& in_factory) override;

 protected:
  virtual bool sort(const Metadata& in_rhs) const override;

 private:
  friend class cereal::access;
  template <class Archive>
  void serialize(Archive& ar, std::uint32_t const version);
};

template <class Archive>
void AssetsFile::serialize(Archive& ar, const std::uint32_t version) {
  if (version == 1)
    ar(
        cereal::make_nvp("Metadata", cereal::base_class<Metadata>(this)),
        CEREAL_NVP(p_name),
        CEREAL_NVP(p_ShowName),
        CEREAL_NVP(p_path_file),
        CEREAL_NVP(p_time),
        CEREAL_NVP(p_user),
        CEREAL_NVP(p_department),
        CEREAL_NVP(p_comment));
}

}  // namespace doodle

CEREAL_REGISTER_TYPE(doodle::AssetsFile)
CEREAL_CLASS_VERSION(doodle::AssetsFile, 1)
