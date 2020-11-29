//
// Created by teXiao on 2020/10/20.
//

#include "freeSynWrap.h"
#include "src/core/coreset.h"

#include "Logger.h"

#include <boost/process.hpp>
#include <iostream>

#include <pugixml.hpp>
#include <QtCore/QFile>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/format.hpp>

CORE_NAMESPACE_S
freeSynWrap::freeSynWrap()
    : hasInclude(false),
      p_tem_golb_(
          std::make_shared<dpath>(boost::filesystem::temp_directory_path())),
      p_tem_config(
          std::make_shared<dpath>(boost::filesystem::temp_directory_path())),
      p_doc_(std::make_shared<pugi::xml_document>()) {
  auto file = QFile(":/resource/template.ffs_batch");
  if (!file.open(QIODevice::ReadOnly)) return;

  auto string = file.readAll().toStdString();
  p_doc_->load_string(string.data());
  file.close();
}

void freeSynWrap::addSynFile(const synPathListPtr &path_list_ptr) {
  for (auto &&item : path_list_ptr) {
    auto pair_parent = p_doc_->root()
                           .select_node("/FreeFileSync/FolderPairs")
                           .node()
                           .append_child("Pair");
    pair_parent.append_child("Left").text().set(
        item.local.generic_string().c_str());
    pair_parent.append_child("Right").text().set(
        createIpPath(item.server.generic_string()).c_str());
  }
}

void freeSynWrap::addInclude(const dstringList &include_list) {
  auto include =
      p_doc_->root().select_node("/FreeFileSync/Filter/Include").node();

  if (include_list.empty()) {
    include.append_child("Item").text().set("*");
    return;
  }
  for (auto &item : include_list) {
    include.append_child("Item").text().set(item.c_str());
  }
  hasInclude = true;
}

void freeSynWrap::addExclude(const dstringList &exclude_list) {
  auto exclude =
      p_doc_->root().select_node("/FreeFileSync/Filter/Exclude").node();
  for (auto &&item : exclude_list) {
    exclude.append_child("Item").text().set(item.c_str());
  }
}

void freeSynWrap::addSubIncludeExclude(const int sub,
                                       const dstringList &include_lsit,
                                       const dstringList &exclude_list) {
  auto pair = p_doc_->root().select_nodes("/FreeFileSync/FolderPairs/Pair");
  if (pair.empty()) return;

  if (sub >= pair.size()) return;
  auto k_pair_sub = pair[sub].node();
  //过滤器
  auto filter = k_pair_sub.append_child("Filter");

  //创建包含选项
  auto include = filter.append_child("Include");
  for (const auto &item : include_lsit) {
    include.append_child("Item").text().set(item.c_str());
  }
  //创建排除选项
  auto exclude = filter.append_child("Exclude");
  for (const auto &item : exclude_list) {
    exclude.append_child("Item").text().set(item.c_str());
  }
  //创建其他必须选项
  auto timeSpan = filter.append_child("TimeSpan");
  timeSpan.append_attribute("Type").set_value("None");
  timeSpan.text().set("0");

  auto sizemin = filter.append_child("SizeMin");
  sizemin.append_attribute("Unit").set_value("None");
  sizemin.text().set("0");
  auto sizeMax = filter.append_child("SizeMax");
  sizeMax.append_attribute("Unit").set_value("None");
  sizeMax.text().set("0");
}

void freeSynWrap::addSubSynchronize(int sub_index,
                                    const syn_set &synchronize_set,
                                    const dpath &versioning_folder) {
  auto k_pair = p_doc_->root()
                    .select_node("/FreeFileSync/FolderPairs")
                    .node()
                    .select_nodes("Pair")[sub_index]
                    .node();
  auto k_syn = k_pair.append_child("Synchronize");
  setSyn(synchronize_set, versioning_folder, &k_syn);
}

void freeSynWrap::setVersioningFolder(const syn_set &synchronize_set,
                                      const dpath &folder) {
  auto k_ver_folder = p_doc_->root()
                          .select_node("/FreeFileSync/Synchronize")
                          .node();  // p_root_.firstChildElement("");
  setSyn(synchronize_set, folder, &k_ver_folder);
}

void freeSynWrap::setSyn(const freeSynWrap::syn_set &set,
                         const dpath &versioning_folder,
                         pugi::xml_node *parent_node) {
  auto k_var = parent_node->append_child("Variant");
  if (set == syn_set::down) {
    k_var.text().set("Custom");
    auto custom = parent_node->append_child("CustomDirections");
    custom.append_child("LeftOnly").text().set("left");
    custom.append_child("RightOnly").text().set("left");
    custom.append_child("LeftNewer").text().set("left");
    custom.append_child("RightNewer").text().set("left");
    custom.append_child("Different").text().set("none");
    custom.append_child("Conflict").text().set("left");
  } else if (set == syn_set::upload) {
    k_var.text().set("Update");
  } else if (set == syn_set::twoWay) {
    k_var.text().set("TwoWay");
  }
  parent_node->append_child("DetectMovedFiles").text().set("false");
  parent_node->append_child("DeletionPolicy").text().set("Versioning");
  auto k_var_dolder = parent_node->append_child("VersioningFolder");
  k_var_dolder.append_attribute("Style").set_value("TimeStamp-Folder");
  k_var_dolder.text().set(
      createIpPath(versioning_folder.generic_string()).c_str());
}

void freeSynWrap::copyGlobSetting() {
  auto k_golb_sett = QFile(":/resource/_GlobalSettings.xml");
  boost::format str("_GlobalSettings_%s.xml");
  str % boost::filesystem::unique_path("%%%%_%%").filename().generic_string();
  *p_tem_golb_ = *p_tem_golb_ / str.str();

  boost::filesystem::ofstream kOfstream;
  kOfstream.open(*p_tem_golb_);
  if (k_golb_sett.open(QIODevice::ReadOnly)) {
    kOfstream << k_golb_sett.readAll().data();
    k_golb_sett.close();
  }
  kOfstream.close();
}

bool freeSynWrap::write() {
  copyGlobSetting();
  boost::format str("doodle_%s.ffs_batch");
  DOODLE_LOG_INFO
      << boost::filesystem::unique_path("%%%%_%%").generic_string().c_str();
  str % boost::filesystem::unique_path("%%%%_%%").filename().generic_string();

  *p_tem_config = *p_tem_config / str.str();
  boost::filesystem::ofstream kofstem;
  kofstem.open(*p_tem_config);
  p_doc_->save(kofstem, "\t", pugi::format_default, pugi::encoding_utf8);

  kofstem.close();
  DOODLE_LOG_INFO << "写入配置文件" << p_tem_config->generic_string().c_str();
  return true;
}

bool freeSynWrap::run() {
  if (!hasInclude) addInclude(dstringList{});
  write();
  auto com_arg = boost::format("FreeFileSync.exe %s %s");
  com_arg % p_tem_config->generic_string() % p_tem_golb_->generic_string();
  DOODLE_LOG_INFO << "命令  " << com_arg.str().c_str();
  auto env = boost::this_process::environment();
  env["PATH"] += R"(C:\Program Files\FreeFileSync\)";

  boost::process::spawn(com_arg.str(), env);
  return true;
}

dstring freeSynWrap::decode64(const dstring &val) {
  using namespace boost::archive::iterators;
  using It =
      transform_width<binary_from_base64<std::string::const_iterator>, 8, 6>;
  return boost::algorithm::trim_right_copy_if(
      std::string(It(std::begin(val)), It(std::end(val))),
      [](char c) { return c == '\0'; });
}

dstring freeSynWrap::encode64(const dstring &val) {
  using namespace boost::archive::iterators;
  using It =
      base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>;
  auto tmp = std::string(It(std::begin(val)), It(std::end(val)));
  return tmp.append((3 - val.size() % 3) % 3, '=');
}

dstring freeSynWrap::createIpPath(const dstring &val) {
  //带密码和永华名称的访问
  // auto serverPath = boost::format("ftp://%s@%s:21%s|pass64=%s");
  // auto &set = coreSet::getSet();
  // serverPath % (set.getProjectname() + set.getUser_en()) % set.getIpFtp() %
  //     val % encode64(set.getUser_en());

  // DOODLE_LOG_INFO << serverPath.str().c_str();
  //不带密码的访问
  auto serverPath = boost::format("ftp://%s@%s:21%s");
  auto &set = coreSet::getSet();
  serverPath % set.getProjectname() % set.getIpFtp() % val;

  DOODLE_LOG_INFO << serverPath.str().c_str();
  return serverPath.str();
}

freeSynWrap::~freeSynWrap() {
  //  boost::filesystem::remove(*p_tem_config);
  //  boost::filesystem::remove(*p_tem_golb_);
}

CORE_NAMESPACE_E