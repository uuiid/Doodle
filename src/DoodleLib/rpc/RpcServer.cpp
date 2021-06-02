//
// Created by TD on 2021/5/25.
//

#include <DoodleLib/core/CoreSql.h>
#include <DoodleLib/rpc/RpcServer.h>
//#include <DoodleLib/Exception/Exception.h>
//#include <DoodleLib/core/ContainerDevice.h>
//#include <DoodleLib/Metadata/Metadata.h>
#include <Logger/Logger.h>
#include <core/MetadataTabSql.h>
#include <google/protobuf/util/time_util.h>
#include <grpcpp/server_builder.h>
#include <sqlpp11/mysql/mysql.h>
#include <sqlpp11/sqlpp11.h>

#include <cereal/archives/portable_binary.hpp>

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
    //    auto time   = google::protobuf::util::TimeUtil::TimeTToTimestamp(std::chrono::system_clock::to_time_t(k_time));
    *k_item = google::protobuf::util::TimeUtil::TimeTToTimestamp(std::chrono::system_clock::to_time_t(k_time));

    auto k_path = getPath(row.uuidPath.value());
    k_ifstream.open(k_path, std::ios::in | std::ios::binary);
    if (k_ifstream.is_open() && k_ifstream.good()) {
      auto k_any = k_data->mutable_metadata_cereal();
      k_any->set_value(std::string{
          std::istreambuf_iterator<char>(k_ifstream),
          std::istreambuf_iterator<char>()});
    } else {
      continue;
    }

    k_ifstream.close();
  }
  // /// 我们在数据库中没有任何项目时添加以前占位项目， 以防grpc出错
  // if (response->data_size() == 0) {
  //   response->add_data()->set_id(-1);
  // }

  return grpc::Status::OK;
}
grpc::Status RpcServer::GetChild(grpc::ServerContext *context, const DataDb *request, DataVector *response) {
  auto k_conn = CoreSql::Get().getConnection();
  Metadatatab k_tab{};
  auto k_date = response->mutable_data();
  for (const auto &row : (*k_conn)(sqlpp::select(sqlpp::all_of(k_tab))
                                       .from(k_tab)
                                       .where(k_tab.parent == request->id()))) {
    DataDb k_db{};
    k_db.set_id(row.id.value());
    k_db.mutable_parent()->set_value(row.parent.value());
    k_db.set_uuidpath(std::string{row.uuidPath.value()});
    auto k_time      = std::chrono::system_clock::time_point{row.updateTime.value()};
    auto k_timestamp = google::protobuf::util::TimeUtil::TimeTToTimestamp(std::chrono::system_clock::to_time_t(k_time));
    k_db.mutable_update_time()->CopyFrom(k_timestamp);
    k_date->Add(std::move(k_db));
  }
  return grpc::Status::OK;
}
void RpcServer::runServer(int port) {
  ///检查p_server防止重复调用
  if (p_Server)
    return;
  std::string server_address{"localhost:"};
  server_address += std::to_string(port);

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
  if (!p_Server)
    return;
  using namespace std::chrono_literals;
  auto k_time = std::chrono::system_clock::now() + 2s;

  p_Server->Shutdown(k_time);
  p_Server->Wait();
  // std::this_thread::sleep_for(3s);
  p_Server.reset();
}

grpc::Status RpcServer::GetMetadata(grpc::ServerContext *context, const DataDb *request, DataDb *response) {
  auto k_conn = CoreSql::Get().getConnection();
  Metadatatab k_tab{};
  for (const auto &row : (*k_conn)(sqlpp::select(sqlpp::all_of(k_tab))
                                       .from(k_tab)
                                       .where(k_tab.id == request->id()))) {
    ///设置一些普遍值
    response->set_id(row.id.value());
    response->mutable_parent()->set_value(row.parent.value());
    //    response->set_uuidpath(std::string{row.uuidPath.value()});
    //
    //    auto k_time = std::chrono::system_clock::time_point{row.updateTime.value()};
    //    auto k_timestamp = google::protobuf::util::TimeUtil::TimeTToTimestamp(std::chrono::system_clock::to_time_t(k_time));
    //    response->mutable_update_time()->CopyFrom(k_timestamp);
    FSys::ifstream k_ifstream{row.uuidPath.value(), std::ios::binary | std::ios::in};
    if (k_ifstream.is_open() && k_ifstream.good()) {
      auto k_any = response->mutable_metadata_cereal();
      k_any->set_value(std::string{
          std::istreambuf_iterator<char>(k_ifstream),
          std::istreambuf_iterator<char>()});
    } else {
      /// 这里我们主动取消
      return grpc::Status::CANCELLED;
    }
  }

  return grpc::Status::OK;
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
    FSys::create_directories(path.parent_path());

  FSys::ofstream k_file{path, std::ios::out | std::ios::binary};
  if (k_file.is_open() && k_file.good()) {
    auto k_data = request->metadata_cereal().value();
    k_file.write(k_data.data(), k_data.size());
  } else {
    return {grpc::StatusCode::FAILED_PRECONDITION, "打开文件错误"};
  }

  return grpc::Status::OK;
}
grpc::Status RpcServer::DeleteMetadata(grpc::ServerContext *context, const DataDb *request, DataDb *response) {
  auto k_conn = CoreSql::Get().getConnection();
  Metadatatab k_tab{};

  auto id = (*k_conn)(sqlpp::remove_from(k_tab).where(k_tab.id == request->id()));
  response->set_id(id);
  return grpc::Status::OK;
}

}  // namespace doodle
