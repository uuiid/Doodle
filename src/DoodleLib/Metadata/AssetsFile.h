//
// Created by TD on 2021/5/7.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Metadata/Metadata.h>

namespace doodle {
class DOODLELIB_API AssetsFile : public Metadata {
  std::string p_name;
  std::string p_ShowName;
  FSys::path p_path_file;
  std::chrono::time_point<std::chrono::system_clock> p_time;
  std::string p_user;
  std::string p_department;
  std::vector<CommentPtr> p_comment;


 public:
  AssetsFile();
  explicit AssetsFile(std::weak_ptr<Metadata> in_metadata, std::string name = {}, std::string showName = {});
  [[nodiscard]] std::string str() const override;
  [[nodiscard]] std::string showStr() const override;

  [[nodiscard]] const std::chrono::time_point<std::chrono::system_clock>& getTime() const;
  void setTime(const std::chrono::time_point<std::chrono::system_clock>& in_time);
  [[nodiscard]] const std::string& getUser() const;
  void setUser(const std::string& in_user);
  [[nodiscard]] const std::string& getDepartment() const;
  void setDepartment(const std::string& in_department);

  [[nodiscard]] const std::vector<CommentPtr>& getComment() const;
  void setComment(const std::vector<CommentPtr>& in_comment);
  void addComment(const CommentPtr& in_comment);

  void load(const MetadataFactoryPtr& in_factory) override;
  void save(const MetadataFactoryPtr& in_factory) override;
  bool operator<(const AssetsFile& in_rhs) const;
  bool operator>(const AssetsFile& in_rhs) const;
  bool operator<=(const AssetsFile& in_rhs) const;
  bool operator>=(const AssetsFile& in_rhs) const;
  virtual void createMenu(ContextMenu* in_contextMenu) override;

 protected:
  virtual bool sort(const Metadata& in_rhs) const override;
  void modifyParent(const std::shared_ptr<Metadata> &in_old_parent) override;

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
        p_name,
        p_ShowName,
        p_path_file);
}

}  // namespace doodle

CEREAL_REGISTER_TYPE(doodle::AssetsFile)
CEREAL_CLASS_VERSION(doodle::AssetsFile, 1)
