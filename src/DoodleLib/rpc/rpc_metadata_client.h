//
// Created by TD on 2021/5/25.
//

#pragma once
#include <DoodleLib/Metadata/metadata.h>
#include <DoodleLib/doodle_lib_fwd.h>
#include <DoodleLib/generate/rpc/metadata_server.grpc.pb.h>
#include <DoodleLib/libWarp/protobuf_warp.h>
#include <grpcpp/channel.h>

namespace doodle {
namespace rpc_filter {
class filter : details::no_copy {
 public:
  using time_point = chrono::local_time_pos;

 private:
  std::optional<std::int64_t> _id;
  std::optional<std::int64_t> _parent_id;
  std::optional<metadata::meta_type> _meta_type;
  std::optional<time_point> _begin;
  std::optional<time_point> _end;

 public:
  filter();

  void set_id(uint64_t in_id);
  void set_parent_id(std::int64_t in_patent_id);
  void set_meta_type(metadata::meta_type in_meta_type);
  void set_begin_time(const time_point& in_time);
  void set_end_time(const time_point& in_time);
  void set_range(const time_point& in_begin, const time_point& in_end);
  void set_range(const std::pair<time_point, time_point>& in_time);

  void reset();
  explicit operator metadata_database_filter() const;
};

using rpc_filter_ptr = std::shared_ptr<filter>;
}  // namespace rpc_filter
/**
 * @brief rpc客户端
 * @warning 这个类在导出的时候使用会报错, 在grpc库中会报空指针错误, 所有不可以在外部使用
 */
class DOODLELIB_API rpc_metadata_client {
  std::unique_ptr<metadata_server::Stub> p_stub;
  // std::shared_ptr<grpc::Channel> p_channel;

 public:
  explicit rpc_metadata_client(const std::shared_ptr<grpc::Channel>& in_channel);

  [[nodiscard]] std::vector<project_ptr> get_project();
  /**
   * @brief 这个函数时获得子项， 但是我们获得数据库中的数据也就是id和父id填充数据并且也获取他们的序列化数据
   * 
   * @param in_metadataPtr 要获得子物体的物体的数据
   * @return std::vector<MetadataPtr>  子物体数据集合
   */
  [[nodiscard]] std::vector<metadata_ptr> get_child(const metadata_const_ptr& in_metadataPtr);

  // /**
  //  * @brief 这个不是获得是数据库中的数据， 是获得服务器中序列化的数据
  //  * 这个函数是 RpcMetadataClient::GetChild 的单项函数
  //  *
  //  * @param in_metadataPtr 要获得的数据对象
  //  */
  // void GetMetadata(const MetadataPtr& in_metadataPtr);

  /**
   * @brief 这里是插入数据库数据
   * 插入时回去检查id是否大于0， 大于0则代表已近插入我们将取消插入 
   * 
   * @param in_metadataPtr 要插入的数据
   */
  void install_metadata(const metadata_ptr& in_metadataPtr);
  void delete_metadata(const metadata_const_ptr& in_metadataPtr);
  void update_metadata(const metadata_const_ptr& in_metadataPtr, bool b_update_parent_id);
  [[nodiscard]] std::vector<metadata_ptr> filter_metadata(const rpc_filter::rpc_filter_ptr& in_filter_ptr);
};
}  // namespace doodle
