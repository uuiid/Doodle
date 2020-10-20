//
// Created by teXiao on 2020/10/20.
//
#include "core_global.h"

#include <QDomDocument>
#include <QTemporaryFile>

CORE_NAMESPACE_S
class CORE_EXPORT freeSynWrap {
Q_GADGET
 public:
  freeSynWrap();
  enum class syn_set{
    down = 0,
    upload = 1,
    twoWay = 2
  };
  bool write();
  //添加同步文件夹
  void addSynFile(const synPathListPtr & path_list_ptr);
  //添加包含和排除文件夹
  void addInclude(const QStringList & include_list);
  void addExclude(const QStringList & exclude_list);
  //为每个文件夹单独设置文件夹
  void addSubIncludeExclude(const int sub,const QStringList& include_lsit,
                            const QStringList &exclude_list);
  //为每个子目设置备份文件夹
  void addSubSynchronize(int sub_index,
                         const syn_set & synchronize_set,
                         const QString & versioning_folder
                         );
  //设置同步方式和备份文件夹
  void setVersioningFolder(const syn_set & synchronize_set,const QString & folder);
 private:
  //设置文件夹的同步方式和备份
  void setSyn(const syn_set & set,
              const QString & versioning_folder,
              QDomNode &parent_node
              );
  void copyGlobSetting();
  QDomDocument p_doc_;
  QDomElement p_root_;
  QFile p_file_;
  QTemporaryFile p_tem_golb_;
  QTemporaryFile p_tem_config;
};
CORE_NAMESPACE_E