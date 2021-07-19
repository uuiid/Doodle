//
// Created by TD on 2021/5/25.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/libWarp/protobuf_warp.h>
#include <Metadata/Metadata.h>
#include <MetadataServer.grpc.pb.h>
#include <grpcpp/channel.h>

namespace doodle {
namespace rpc_filter {
class filter : details::no_copy {
 public:
  using time_point = std::chrono::time_point<std::chrono::system_clock>;

 private:
  std::optional<std::int64_t> _id;
  std::optional<std::int64_t> _parent_id;
  std::optional<Metadata::meta_type> _meta_type;
  std::optional<time_point> _begin;
  std::optional<time_point> _end;

 public:
  filter();

  void set_id(std::int64_t in_id);
  void set_parent_id(std::int64_t in_patent_id);
  void set_meta_type(Metadata::meta_type in_meta_type);
  void set_begin_time(const time_point& in_time);
  void set_end_time(const time_point& in_time);
  void set_range(const time_point& in_begin, const time_point& in_end);

  explicit operator DataDb_Filter() const;
};

using rpc_filter_ptr = std::shared_ptr<filter>;
}  // namespace rpc_filter
/**
 * @brief rpc客户端
 * @warning 这个类在导出的时候使用会报错, 在grpc库中会报空指针错误, 所有不可以在外部使用
 */
class DOODLELIB_API RpcMetadataClient {
  std::unique_ptr<MetadataServer::Stub> p_stub;
  // std::shared_ptr<grpc::Channel> p_channel;

 public:
  explicit RpcMetadataClient(const std::shared_ptr<grpc::Channel>& in_channel);

  [[nodiscard]] std::vector<ProjectPtr> GetProject();
  /**
   * @brief 这个函数时获得子项， 但是我们获得数据库中的数据也就是id和父id填充数据并且也获取他们的序列化数据
   * 
   * @param in_metadataPtr 要获得子物体的物体的数据
   * @return std::vector<MetadataPtr>  子物体数据集合
   */
  [[nodiscard]] std::vector<MetadataPtr> GetChild(const MetadataConstPtr& in_metadataPtr);

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
  void InstallMetadata(const MetadataPtr& in_metadataPtr);
  void DeleteMetadata(const MetadataConstPtr& in_metadataPtr);
  void UpdateMetadata(const MetadataConstPtr& in_metadataPtr);
  void UpdateMetadata(const MetadataConstPtr& in_metadataPtr, bool b_update_parent_id);
  [[nodiscard]] std::vector<MetadataPtr> FilterMetadata(const rpc_filter::rpc_filter_ptr& in_filter_ptr);
};
}  // namespace doodle
