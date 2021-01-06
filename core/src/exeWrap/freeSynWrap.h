//
// Created by teXiao on 2020/10/20.
//
#include "core_global.h"

namespace pugi {
class xml_document;
class xml_node;
}  // namespace pugi

DOODLE_NAMESPACE_S
class CORE_API freeSynWrap : public boost::noncopyable_::noncopyable {
 public:
  freeSynWrap();
  enum class syn_set {
    down   = 0,
    upload = 1,
    twoWay = 2
  };
  ~freeSynWrap();
  bool run();

  //添加同步文件夹
  void addSynFile(const synPathListPtr &path_list_ptr);
  //添加包含和排除文件夹
  void addInclude(const dstringList &include_list);
  void addExclude(const dstringList &exclude_list);
  //为每个文件夹单独设置文件夹
  void addSubIncludeExclude(const int sub, const dstringList &include_lsit,
                            const dstringList &exclude_list);
  //为每个子目设置备份文件夹和同步备份方式
  void addSubSynchronize(int sub_index,
                         const syn_set &synchronize_set,
                         const dpath &versioning_folder);
  //设置同步方式和备份文件夹
  void setVersioningFolder(const syn_set &synchronize_set, const dpath &folder);

 private:
  //设置文件夹的同步方式和备份
  void setSyn(const syn_set &set,
              const dpath &versioning_folder,
              pugi::xml_node *parent_node);
  bool write();
  void copyGlobSetting();
  dstring createIpPath(const dstring &val);
  static dstring decode64(const dstring &val);
  static dstring encode64(const dstring &val);

  bool hasInclude;

  std::shared_ptr<pugi::xml_document> p_doc_;
  dpathPtr p_tem_golb_;
  dpathPtr p_tem_config;
};
DOODLE_NAMESPACE_E