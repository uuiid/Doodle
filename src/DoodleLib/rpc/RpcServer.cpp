//
// Created by TD on 2021/5/25.
//

#include <DoodleLib/rpc/RpcServer.h>
#include <DoodleLib/core/CoreSql.h>
#include <DoodleLib/Exception/Exception.h>
#include <Logger/Logger.h>

#include <core/MetadataTabSql.h>
#include <sqlpp11/sqlpp11.h>
#include <google/protobuf/util/time_util.h>
#include <grpcpp/server_builder.h>
//#include <grpcpp/server_context.h>
//#include <grpcpp/server.h>
//#include <grpcpp/security/server_credentials.h>

namespace doodle {

std::unique_ptr<grpc::Server> RpcServer::p_Server{};

RpcServer::RpcServer()
    : p_set(CoreSet::getSet()) {
}

grpc::Status RpcServer::GetProject(grpc::ServerContext *context, const google::protobuf::Empty *request, DataVector *response) {
  auto k_conn = CoreSql::Get().getConnection();
  Metadatatab k_tab{};

  FSys::ifstream k_ifstream{};
  for (const auto &row : (*k_conn)(sqlpp::select(sqlpp::all_of(k_tab))
                                       .from(k_tab)
                                       .where(k_tab.parent.is_null()))) {
    auto k_data = response->add_data();
    k_data->set_id(row.id.value());
    k_data->set_uuidpath(std::string{row.uuidPath.value()});
    k_data->update_time();
    auto k_item = k_data->mutable_update_time();
    /// 这个到时候还要重新斟酌一下，有没有更快的转换方案
    auto k_time = std::chrono::system_clock::time_point{row.updateTime.value()};
    auto time   = google::protobuf::util::TimeUtil::TimeTToTimestamp(std::chrono::system_clock::to_time_t(k_time));
    *k_item     = google::protobuf::util::TimeUtil::TimeTToTimestamp(std::chrono::system_clock::to_time_t(k_time));

    auto k_path = getPath(row.uuidPath.value());
    auto k_size = FSys::file_size(k_path);
    k_ifstream.open(k_path, std::ios::in | std::ios::binary);
    if (k_ifstream.is_open()) {
      auto k_any = k_data->mutable_metadata_cereal();
      //      k_any->set_type_url("metadata_cereal");
      k_any->set_value(k_ifstream.rdbuf(), k_size);
    }
  }

  return {};
}
grpc::Status RpcServer::GetChild(grpc::ServerContext *context, const DataDb *request, DataVector *response) {
  return {};
}
void RpcServer::runServer() {
  ///检查p_server防止重复调用
  if (p_Server)
    return;
  std::string server_address{"127.0.0.1:60999"};
  RpcServer service{};

  grpc::ServerBuilder k_builder{};
  k_builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  k_builder.RegisterService(&service);

  //  auto t = k_builder.BuildAndStart();
  p_Server = std::move(k_builder.BuildAndStart());
  DOODLE_LOG_INFO("Server listening on " << server_address);
  p_Server->Wait();
}

void RpcServer::stop() {
  p_Server->Shutdown();
  p_Server.reset();
}

FSys::path RpcServer::getPath(const std::string &in_string) const {
  if (in_string.empty())
    throw DoodleError{"str 是空的"};
  return p_set.getCacheRoot() / in_string;
}
grpc::Status RpcServer::GetMetadata(grpc::ServerContext *context, const DataDb *request, DataDb *response) {
  return grpc::Status();
}
grpc::Status RpcServer::InstallMetadata(grpc::ServerContext *context, const DataDb *request, DataDb *response) {
  auto k_conn = CoreSql::Get().getConnection();
  Metadatatab k_tab{};

  auto k_in = sqlpp::dynamic_insert_into(*k_conn, k_tab).dynamic_set();
  k_in.insert_list.add(k_tab.uuidPath = request->uuidpath());
  if (request->has_parent()) {
    k_in.insert_list.add(k_tab.parent = request->parent().value());
  }
  auto k_id = (*k_conn)(k_in);

  if (k_id < 0)
    return {grpc::StatusCode::RESOURCE_EXHAUSTED, "插入数据库失败"};

  response->set_id(k_id);

  auto path = getPath(request->uuidpath());
  if (!FSys::exists(path.parent_path()))
    FSys::create_directory(path.parent_path());

  FSys::ofstream k_file{path, std::ios::out | std::ios::binary};
  if (k_file.is_open()) {
    k_file << request->metadata_cereal().value();
  } else {
    return {grpc::StatusCode::FAILED_PRECONDITION, "打开文件错误"};
  }

  return grpc::Status::OK;
}
grpc::Status RpcServer::DeleteMetadata(grpc::ServerContext *context, const DataDb *request, DataDb *response) {
  return grpc::Status();
}
//FSys::path RpcServer::getPath(uint64_t id, const std::string& in_string) const {
//  auto k_path = p_set.getCacheRoot();
//  if (in_string.empty())
//    throw DoodleError{"str 是空的"};
//  return k_path / std::to_string(id) / in_string.substr(0, 3) / in_string;
//}

}  // namespace doodle
