//
// Created by TD on 2021/5/25.
//

#include <DoodleLib/core/CoreSql.h>
#include <DoodleLib/core/MetadataTabSql.h>
#include <DoodleLib/rpc/RpcServer.h>
#include <DoodleLib/core/CoreSet.h>

#include <sqlpp11/sqlpp11.h>
#include <google/protobuf/util/time_util.h>

namespace doodle{
RpcServer::RpcServer() {
}

::grpc::Status RpcServer::GetProject(::grpc::ServerContext* context, const ::google::protobuf::Empty* request, ::doodle::DataVector* response) {
  auto k_conn = CoreSql::Get().getConnection();

  MetadataTab k_tab{};

  FSys::ifstream k_ifstream{};
  for(const auto& row :(*k_conn)(sqlpp::select(sqlpp::all_of(k_tab))
                                     .from(k_tab)
                                     .where(k_tab.parent.is_null()))){
    auto k_data = response->add_data();
    k_data->set_id(row.id.value());
    k_data->set_parent(row.parent.value());
    k_data->set_uuidpath(row.uuidPath.value());
    k_data->update_time();
    auto k_time = std::chrono::system_clock::time_point{row.updateTime.value()};

    auto time =  google::protobuf::util::TimeUtil::TimeTToTimestamp(std::chrono::system_clock::to_time_t(k_time));
    k_data->set_allocated_update_time(&time);
    k_ifstream.open(FSys::path{std::string{row.uuidPath.value()}},std::ios::in|std::ios::binary);
    if(k_ifstream.is_open()){

    }
  }

  return {};
}
::grpc::Status RpcServer::GetChild(::grpc::ServerContext* context, const ::doodle::DataDb* request, ::doodle::DataVector* response) {
  return {};
}
void RpcServer::runServer() {
}
void RpcServer::stop() {
}

}
