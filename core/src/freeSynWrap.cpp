//
// Created by teXiao on 2020/10/20.
//

#include "freeSynWrap.h"
#include "coreset.h"

#include "Logger.h"

#include <QStringList>
#include <QTextStream>

CORE_NAMESPACE_S
freeSynWrap::freeSynWrap() {
  auto file = QFile(":/template.ffs_batch");
  if (!file.open(QIODevice::ReadOnly)) return;
  if (!p_doc_.setContent(&file)) {
    file.close();
    return;
  }
  file.close();

  p_root_ = p_doc_.documentElement();
}
void freeSynWrap::addSynFile(const synPathListPtr &path_list_ptr) {
  for (auto &&x : path_list_ptr) {
    auto pair_parent = p_root_.firstChildElement("FolderPairs")
        .appendChild(p_doc_.createElement("Pair")).toElement();
    pair_parent.appendChild(
        p_doc_.createElement("Left")
    ).appendChild(
        p_doc_.createTextNode(x.local)
    );
    pair_parent.appendChild(
        p_doc_.createElement("Right")
    ).appendChild(
        p_doc_.createTextNode(x.server)
    );
  }
}
void freeSynWrap::addInclude(const QStringList &include_list) {
  auto include = p_root_.elementsByTagName("Include").item(0);

  if (include_list.empty())
    include.appendChild(
            p_doc_.createElement("Item"))
        .appendChild(p_doc_.createTextNode("*"));
  for (auto &&x :include_list) {
    include.appendChild(
        p_doc_.createElement("Item")
    ).appendChild(p_doc_.createTextNode(x));
  }
  include.parentNode().replaceChild(
      p_root_.elementsByTagName("Include").item(0),
      include
  );
}
void freeSynWrap::addExclude(const QStringList &exclude_list) {
  auto exclude = p_root_.elementsByTagName("Exclude").item(0);
  for (auto &&x :exclude_list) {
    exclude.appendChild(
        p_doc_.createElement("Item")
    ).appendChild(
        p_doc_.createTextNode(x)
    );
  }
  exclude.parentNode().replaceChild(
      p_root_.elementsByTagName("Exclude").item(0),
      exclude
  );
}
void freeSynWrap::addSubIncludeExclude(const int sub, const QStringList &include_lsit,
                                       const QStringList &exclude_list) {
  auto pair = p_root_.elementsByTagName("Pair");
  if (pair.isEmpty()) return;

  auto k_pair_sub = pair.item(sub).toElement();
  auto k_filter = p_doc_.createElement("Filter");
  //创建包含选项
  k_filter.appendChild(
      p_doc_.createElement("Include")
  );
  for (auto &&x:include_lsit) {
    k_filter.firstChild().appendChild(p_doc_.createElement("Item"))
        .appendChild(p_doc_.createTextNode(x));
  }
  //创建排除选项
  k_filter.appendChild(p_doc_.createElement("Exclude"));
  for (auto &&x:exclude_list) {
    k_filter.lastChild().appendChild(p_doc_.createElement("Item"))
        .appendChild(p_doc_.createTextNode(x));
  }
  //创建其他必须选项
  auto timespan = k_filter.appendChild(p_doc_.createElement("TimeSpan"))
      .toElement();
  timespan.setAttribute("Type", "None");
  timespan.appendChild(p_doc_.createTextNode("0"));

  auto sizemin = k_filter.appendChild(p_doc_.createElement("SizeMin")).toElement();
  sizemin.appendChild(p_doc_.createTextNode("0"));
  sizemin.setAttribute("Unit", "None");

  auto sizeMax = k_filter.appendChild(p_doc_.createElement("SizeMax")).toElement();
  sizeMax.appendChild(p_doc_.createTextNode("0"));
  sizeMax.setAttribute("Unit", "None");

  k_pair_sub.appendChild(k_filter);
  k_pair_sub.parentNode().replaceChild(k_pair_sub, pair.item(sub));
}
void freeSynWrap::addSubSynchronize(int sub_index, const syn_set &synchronize_set, const QString &versioning_folder) {

  auto k_pair = p_root_.firstChildElement("FolderPairs").elementsByTagName("Pair").item(sub_index);
  auto k_syn = k_pair.appendChild(p_doc_.createElement("Synchronize")).toElement();
  setSyn(synchronize_set, versioning_folder, k_syn);
}

void freeSynWrap::setVersioningFolder(const syn_set &synchronize_set, const QString &folder) {
  auto k_ver_folder = p_root_.firstChildElement("Synchronize");
  setSyn(synchronize_set, folder, k_ver_folder);
}

void freeSynWrap::setSyn(const freeSynWrap::syn_set &set,
                         const QString &versioning_folder,
                         QDomNode &parent_node) {
  auto k_var = parent_node.appendChild(p_doc_.createElement("Variant")).toElement();
  if (set == syn_set::down) {
    k_var.appendChild(p_doc_.createTextNode("Custom"));
    auto custom = parent_node.appendChild(p_doc_.createElement("CustomDirections"));
    custom.appendChild(p_doc_.createElement("LeftOnly"))
        .appendChild(p_doc_.createTextNode("left"));
    custom.appendChild(p_doc_.createElement("RightOnly"))
        .appendChild(p_doc_.createTextNode("left"));
    custom.appendChild(p_doc_.createElement("LeftNewer"))
        .appendChild(p_doc_.createTextNode("left"));
    custom.appendChild(p_doc_.createElement("RightNewer"))
        .appendChild(p_doc_.createTextNode("left"));
    custom.appendChild(p_doc_.createElement("Different"))
        .appendChild(p_doc_.createTextNode("none"));
    custom.appendChild(p_doc_.createElement("Conflict"))
        .appendChild(p_doc_.createTextNode("left"));
  } else if (set == syn_set::upload) {
    k_var.appendChild(p_doc_.createTextNode("Update"));
  } else if (set == syn_set::twoWay) {
    k_var.appendChild(p_doc_.createTextNode("TwoWay"));
  }
  parent_node.appendChild(p_doc_.createElement("DetectMovedFiles"))
      .appendChild(p_doc_.createTextNode("false"));
  parent_node.appendChild(p_doc_.createElement("DeletionPolicy"))
      .appendChild(p_doc_.createTextNode("Versioning"));
  auto k_ver_dolder = parent_node.appendChild(p_doc_.createElement("VersioningFolder")).toElement();
  k_ver_dolder.setAttribute("Style", "TimeStamp-Folder");
  k_ver_dolder.appendChild(p_doc_.createTextNode(versioning_folder));
}
void freeSynWrap::copyGlobSetting() {
  auto k_golb_sett = QFile(":/_GlobalSettings.xml");
  if (p_tem_golb_.open()) {
    if (k_golb_sett.open(QIODevice::ReadOnly))
      p_tem_golb_.write(k_golb_sett.readAll());
    k_golb_sett.close();
  }
  p_tem_golb_.close();
}
bool freeSynWrap::write() {
  p_tem_config.setFileTemplate("doodle_XXXXXX.ffs_batch");
  if (p_tem_config.open()) {
    QTextStream k_stream(&p_tem_config);
    p_doc_.save(k_stream, 4);
  }
  p_tem_config.close();
  DOODLE_LOG_INFO << "写入配置文件" << QFileInfo(p_tem_config).absoluteFilePath();
  return true;
}

CORE_NAMESPACE_E