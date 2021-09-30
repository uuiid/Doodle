//
// Created by TD on 2021/6/9.
//

#include "rpc_file_system_client.h"

#include <doodle_lib/Exception/exception.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/thread_pool/thread_pool.h>
#include <Logger/logger.h>
#include <core/doodle_lib.h>
#include <google/protobuf/util/time_util.h>
#include <grpcpp/grpcpp.h>
#include <thread_pool/long_term.h>

#include <boost/numeric/conversion/cast.hpp>

namespace doodle {
std::tuple<std::optional<bool>, std::optional<bool>, bool, std::size_t> rpc_file_system_client::compare_file_is_down(const FSys::path& in_local_path, const FSys::path& in_server_path) {
  auto k_l_ex  = FSys::exists(in_local_path);
  auto k_l_dir = k_l_ex && FSys::is_directory(in_local_path);
  auto k_l_sz  = k_l_ex ? FSys::file_size(in_local_path) : 0;
  auto k_l_ti  = k_l_ex
                     ? FSys::last_write_time_point(in_local_path)
                     : chrono::sys_time_pos{};

  auto [k_s_sz, k_s_ex, k_s_ti, k_s_dir] = get_info(in_server_path);
  if (k_l_ex && k_s_ex) {                                   /// 本地文件和服务器文件都存在
    if (k_l_dir || k_s_dir) {                               /// 两个任意一个为目录我们都没有办法确定上传和下载的方案
      return {{}, {}, k_s_ex, 0};                           /// 所以返回无
    }                                                       ///
    auto k_l_hash = FSys::file_hash_sha224(in_local_path);  ///
    auto k_s_hash = get_hash(in_server_path);               ///
    if (k_l_hash == k_s_hash) {                             /// 我们比较两个文件的时间和大小都相同的时候， 直接表示两个文件相同， 既不上穿也不下载
      return {true, {}, k_s_ex, k_l_sz};                    /// 所以返回无
    } else {                                                ///
      if (k_l_ti < k_s_ti) {                                /// 本地文件的修改时间小于服务器时间 那么就是本地文件比较旧 服务器文件新， 需要下载
        return {false, true, k_s_ex, k_s_sz};               /// 返回 true
      } else {                                              /// 本地文件的修改时间大于服务器时间 那么就是本地文件比较新 服务时间比较旧, 需要上传
        return {false, false, k_s_ex, k_l_sz};              /// 返回 false
      }                                                     ///
    }                                                       ///
  } else if (k_l_ex) {                                      /// 本地文件存在和服务器文件不存在
    return {false, false, k_s_ex, k_l_sz};                  /// 返回 false
  } else if (k_s_ex) {                                      /// 本地文件不存在和服务器存在
    return {false, true, k_s_ex, k_s_sz};                   /// 返回 true
  } else {                                                  /// 本地和服务器文件都不存在
    return {{}, {}, k_s_ex, 0};
  }
}

rpc_file_system_client::rpc_file_system_client(const std::shared_ptr<grpc::Channel>& in_channel)
    : p_stub(file_system_server::NewStub(in_channel))
// p_channel(in_channel)
{
}

std::tuple<std::size_t, bool, chrono::sys_time_pos, bool> rpc_file_system_client::get_info(const FSys::path& in_server_path) {
  grpc::ClientContext k_context{};

  file_info_server k_in_info{};
  k_in_info.set_path(in_server_path.generic_string());
  file_info_server k_out_info;
  auto status = p_stub->get_info(&k_context, k_in_info, &k_out_info);
  if (!status.ok())
    throw doodle_error{status.error_message()};

  auto k_t = std::chrono::system_clock::from_time_t(google::protobuf::util::TimeUtil::TimestampToTimeT(k_out_info.update_time()));
  return {k_out_info.size(), k_out_info.exist(), k_t, k_out_info.is_folder()};
}

std::size_t rpc_file_system_client::get_size(const FSys::path& in_server_path) {
  grpc::ClientContext k_context{};

  file_info_server k_in_info{};
  k_in_info.set_path(in_server_path.generic_string());
  file_info_server k_out_info;
  auto status = p_stub->get_size(&k_context, k_in_info, &k_out_info);
  if (!status.ok())
    throw doodle_error{status.error_message()};

  return k_out_info.size();
}

std::tuple<bool, bool> rpc_file_system_client::is_folder(const FSys::path& in_server_path) {
  grpc::ClientContext k_context{};

  file_info_server k_in_info{};
  k_in_info.set_path(in_server_path.generic_string());
  file_info_server k_out_info;
  auto status = p_stub->is_folder(&k_context, k_in_info, &k_out_info);
  if (!status.ok())
    throw doodle_error{status.error_message()};

  return {k_out_info.exist(), k_out_info.is_folder()};
}

chrono::sys_time_pos rpc_file_system_client::get_timestamp(const FSys::path& in_server_path) {
  grpc::ClientContext k_context{};

  file_info_server k_in_info{};
  k_in_info.set_path(in_server_path.generic_string());
  file_info_server k_out_info;
  auto status = p_stub->get_timestamp(&k_context, k_in_info, &k_out_info);
  if (!status.ok())
    throw doodle_error{status.error_message()};
  auto k_t = std::chrono::system_clock::from_time_t(google::protobuf::util::TimeUtil::TimestampToTimeT(k_out_info.update_time()));

  return k_t;
}

bool rpc_file_system_client::is_exist(const FSys::path& in_server_path) {
  grpc::ClientContext k_context{};

  file_info_server k_in_info{};
  k_in_info.set_path(in_server_path.generic_string());
  file_info_server k_out_info;
  auto status = p_stub->is_exist(&k_context, k_in_info, &k_out_info);
  if (!status.ok())
    throw doodle_error{status.error_message()};

  return k_out_info.exist();
}

rpc_file_system_client::trans_file_ptr rpc_file_system_client::download(const FSys::path& in_local_path, const FSys::path& in_server_path) {
  std::unique_ptr<rpc_trans_path> k_ptr{new rpc_trans_path{in_local_path, in_server_path}};
  return download(k_ptr);
}

rpc_file_system_client::trans_file_ptr rpc_file_system_client::upload(const FSys::path& in_local_path, const FSys::path& in_server_path, const FSys::path& in_backup_path) {
  std::unique_ptr<rpc_trans_path> k_ptr{new rpc_trans_path{in_local_path, in_server_path, in_backup_path}};
  return upload(k_ptr);
}
rpc_file_system_client::trans_file_ptr rpc_file_system_client::download(std::vector<std::unique_ptr<rpc_trans_path>>& in_vector) {
  auto k_up = new_object<rpc_trans::trans_files>(this);
  for (auto& k_item : in_vector) {
    k_up->_list.push_back(download(k_item));
  }
  return k_up;
}
rpc_file_system_client::trans_file_ptr rpc_file_system_client::upload(std::vector<std::unique_ptr<rpc_trans_path>>& in_vector) {
  auto k_up = new_object<rpc_trans::trans_files>(this);
  for (auto& k_item : in_vector) {
    k_up->_list.push_back(upload(k_item));
  }
  return k_up;
}
rpc_file_system_client::trans_file_ptr rpc_file_system_client::download(std::unique_ptr<rpc_trans_path>& in_vector) {
  auto [k_ex, k_dir] = is_folder(in_vector->server_path);
  if (!k_ex)
    throw doodle_error{"服务器中不存在文件或者目录"};

  trans_file_ptr k_down;
  if (k_dir)
    k_down = new_object<rpc_trans::down_dir>(this);
  else
    k_down = new_object<rpc_trans::down_file>(this);

  k_down->set_parameter(in_vector);
  return k_down;
}
rpc_file_system_client::trans_file_ptr rpc_file_system_client::upload(std::unique_ptr<rpc_trans_path>& in_vector) {
  if (!FSys::exists(in_vector->local_path))
    throw doodle_error{"本地中不存在文件或者目录"};

  trans_file_ptr k_up;
  if (FSys::is_directory(in_vector->local_path))
    k_up = new_object<rpc_trans::up_dir>(this);
  else
    k_up = new_object<rpc_trans::up_file>(this);

  k_up->set_parameter(in_vector);
  return k_up;
}
std::string rpc_file_system_client::get_hash(const FSys::path& in_path) {
  grpc::ClientContext k_context{};

  file_info_server k_in_info{};
  k_in_info.set_path(in_path.generic_string());
  file_info_server k_out_info;
  auto status = p_stub->get_hash(&k_context, k_in_info, &k_out_info);
  if (!status.ok())
    throw doodle_error{status.error_message()};
  return k_out_info.hash().value();
}

namespace rpc_trans {

trans_file::trans_file(rpc_file_system_client* in_self)
    : _self(in_self),
      _param(),
      _term(new_object<long_term>()),
      _result(),
      _size(0),
      _mutex() {
}
void trans_file::set_parameter(std::unique_ptr<rpc_trans_path>& in_path) {
  _param = std::move(in_path);
}
long_term_ptr trans_file::operator()() {
  _result = doodle_lib::Get().get_thread_pool()->enqueue([this]() {
    try {
      this->run();
    } catch (doodle_error& error) {
      DOODLE_LOG_WARN(error.what());
      _term->sig_progress(1);
      _term->sig_finished();
      _term->sig_message_result(error.what(), long_term::warning);
      throw error;
    }
  });
  return _term;
}
const long_term_ptr& trans_file::get_term() {
  return _term;
}

down_file::down_file(rpc_file_system_client* in_self)
    : trans_file(in_self) {
}

void down_file::run() {
  _term->start();
  auto [k_is_eq, k_is_down, k_s_ex, k_sz] = _self->compare_file_is_down(_param->local_path, _param->server_path);
  if (!k_is_eq) {
    _term->sig_progress(1);
    _term->sig_finished();
    _term->sig_message_result(
        fmt::format("完成 local_path: {} server_path: {}", _param->local_path, _param->server_path), long_term::warning);
    return;
  }

  if ((*k_is_eq)) {
    _term->sig_progress(1);
    _term->sig_finished();
    _term->sig_message_result(
        fmt::format("完成 local_path: {} server_path: {}", _param->local_path, _param->server_path), long_term::warning);
    return;
  }

  if (!FSys::exists(_param->local_path.parent_path()))
    FSys::create_directories(_param->local_path.parent_path());

  grpc::ClientContext k_context{};

  file_info_server k_in_info{};
  file_stream_server k_out_info{};

  FSys::ofstream k_file{_param->local_path, std::ios::out | std::ios::binary};
  if (!k_file)
    throw doodle_error{"not create file"};

  k_in_info.set_path(_param->server_path.generic_string());
  auto k_out = _self->p_stub->download(&k_context, k_in_info);

  const std::size_t k_num2{core_set::get_block_size() > k_sz
                               ? 1
                               : (core_set::get_block_size() / k_sz)};
  while (k_out->Read(&k_out_info)) {
    auto& str = k_out_info.data().value();
    k_file.write(str.data(), str.size());
    _term->sig_progress(rational_int{1, k_num2});
  }

  auto status = k_out->Finish();

  if (!status.ok())
    throw doodle_error{status.error_message()};
  _term->sig_finished();
  _term->sig_message_result(fmt::format("完成 local_path: {} server_path: {}\n", _param->local_path, _param->server_path),long_term::warning);
}
void down_file::wait() {
  _result.get();
}
up_file::up_file(rpc_file_system_client* in_self)
    : trans_file(in_self) {
}
void up_file::run() {
  _term->start();
  auto [k_is_eq, k_is_down, k_s_ex, k_sz] = _self->compare_file_is_down(_param->local_path, _param->server_path);
  if (!k_is_eq) {
    _term->sig_progress(1);
    _term->sig_finished();
    _term->sig_message_result(fmt::format("完成 local_path: {} server_path: {}\n", _param->local_path, _param->server_path),long_term::warning);
    return;
  }

  if (*k_is_eq) {
    _term->sig_progress(1);
    _term->sig_finished();
    _term->sig_message_result(fmt::format("完成 local_path: {} server_path: {}\n", _param->local_path, _param->server_path),long_term::warning);
    return;
  }

  if (k_s_ex && !_param->backup_path.empty()) {
    grpc::ClientContext k_context{};

    file_info_move_server k_info{};
    file_info_server k_out_info{};
    k_info.mutable_source()->set_path(std::move(_param->server_path.generic_string()));
    k_info.mutable_target()->set_path(std::move(_param->backup_path.generic_string()));

    auto k_s = _self->p_stub->move(&k_context, k_info, &k_out_info);
    if (!k_s.ok()) {
      DOODLE_LOG_WARN(k_s.error_message());
      throw doodle_error(k_s.error_message());
    }
  }

  grpc::ClientContext k_context{};

  file_info_server k_out_info{};
  file_stream_server k_in_info{};

  k_in_info.mutable_info()->set_path(_param->server_path.generic_string());
  auto k_in = _self->p_stub->upload(&k_context, &k_out_info);
  k_in->Write(k_in_info);

  auto s_size = core_set::get_block_size();
  const std::size_t k_num2{s_size > k_sz ? 1 : (s_size / k_sz)};
  FSys::ifstream k_file{_param->local_path, std::ios::in | std::ios::binary};
  if (!k_file)
    throw doodle_error{"not read file"};

  while (k_file) {
    std::string k_value{};
    k_value.resize(s_size);
    k_file.read(k_value.data(), s_size);
    auto k_s = k_file.gcount();
    if (k_s != s_size) {
      k_value.resize(k_s);
      k_value.erase(k_s);
    }

    k_in_info.mutable_data()->set_value(std::move(k_value));
    _term->sig_progress(rational_int(k_num2));
    if (!k_in->Write(k_in_info))
      throw doodle_error{"write stream errors"};
  }
  /// @warning 这里必须调用 WritesDone用来区分写入完成
  k_in->WritesDone();
  auto status = k_in->Finish();
  if (!status.ok())
    throw doodle_error{status.error_message()};
  _term->sig_finished();
  _term->sig_message_result(fmt::format("完成 local_path: {} server_path: {}\n", _param->local_path, _param->server_path),long_term::warning);
}
void up_file::wait() {
  _result.get();
}
down_dir::down_dir(rpc_file_system_client* in_self)
    : trans_file(in_self),
      _down_list() {
}
void down_dir::run() {
  if (!FSys::exists(_param->local_path))
    FSys::create_directories(_param->local_path);

  rpc_trans_path_ptr_list _stack{};
  _stack.push_back(std::move(_param));
  while (!_stack.empty()) {
    auto k_i = down(_stack.back());
    _stack.pop_back();
    _stack.insert(_stack.end(), std::make_move_iterator(k_i.begin()), std::make_move_iterator(k_i.end()));
  }

  for (auto& k_i : _down_list) {
    (*k_i)();
  }
}
rpc_trans_path_ptr_list down_dir::down(const std::unique_ptr<rpc_trans_path>& in_path) {
  grpc::ClientContext k_context{};

  file_info_server k_in_info{};
  file_info_server k_out_info{};

  k_in_info.set_path(in_path->server_path.generic_string());
  auto k_out = _self->p_stub->get_list(&k_context, k_in_info);

  auto k_prot = doodle_lib::Get().get_thread_pool();

  rpc_trans_path_ptr_list _stack{};
  while (k_out->Read(&k_out_info)) {
    FSys::path k_s_p = k_out_info.path();
    FSys::path k_l_p = in_path->local_path / k_s_p.filename();
    std::unique_ptr<rpc_trans_path> k_ptr{new rpc_trans_path{k_l_p, k_s_p}};
    if (k_out_info.is_folder()) {
      _stack.push_back(std::move(k_ptr));
      //      down(k_ptr);
      //      std::unique_lock lock{p_mutex};
      //      k_future_list.emplace_back(
      //          k_prot->enqueue(
      //              [k_s_p, k_l_p, this](std::vector<std::future<void>>& in_future_list_) {
      //              },
      //              std::ref(k_future_list)));

    } else {
      DOODLE_LOG_DEBUG(fmt::format("准备下载文件: {} <-----  {}", k_l_p, k_s_p))
      auto k_down = _down_list.emplace_back(new_object<down_file>(_self));
      k_down->set_parameter(k_ptr);
    }
  }
  auto status = k_out->Finish();
  if (!status.ok())
    throw doodle_error{status.error_message()};
  return _stack;
}
void down_dir::wait() {
  _result.get();
  for (auto& k_i : _down_list) {
    k_i->_result.get();
  }
}
up_dir::up_dir(rpc_file_system_client* in_self)
    : trans_file(in_self),
      _up_list() {
}
void up_dir::run() {
  if (!FSys::exists(_param->local_path))
    throw doodle_error{"未找到上传文件夹"};

  rpc_trans_path_ptr_list _stack{};
  _stack.push_back(std::move(_param));
  while (!_stack.empty()) {
    auto k_list = update(_stack.back());
    _stack.pop_back();
    _stack.insert(_stack.end(), std::make_move_iterator(k_list.begin()), std::make_move_iterator(k_list.end()));
  }

  for (auto& k_i : _up_list) {
    (*k_i)();
  }
}
rpc_trans_path_ptr_list up_dir::update(const std::unique_ptr<rpc_trans_path>& in_path) {
  std::vector<std::pair<FSys::path, FSys::path>> path_list{};
  auto k_prot = doodle_lib::Get().get_thread_pool();

  rpc_trans_path_ptr_list _stack{};
  for (const auto& k_it : FSys::directory_iterator(in_path->local_path)) {
    FSys::path k_s_p = in_path->server_path / k_it.path().filename();
    auto k_back      = in_path->backup_path / k_it.path().filename();
    auto k_l_p       = k_it.path();
    std::unique_ptr<rpc_trans_path> k_ptr{new rpc_trans_path{k_l_p, k_s_p, k_back}};

    if (FSys::is_directory(k_it)) {
      _stack.push_back(std::move(k_ptr));
      //      std::unique_lock lock{p_mutex};
      //      in_future_list.emplace_back(
      //          k_prot->enqueue(
      //              [k_l_p, k_s_p, this](std::vector<std::future<void>>& in_future_list_) {
      //              },
      //              std::ref(in_future_list)));
    } else {
      DOODLE_LOG_DEBUG(fmt::format("准备上传文件: {} -----> {}", k_l_p, k_s_p))
      auto k_up = _up_list.emplace_back(new_object<up_file>(_self));
      k_up->set_parameter(k_ptr);
    }
  }
  return _stack;
}
void up_dir::wait() {
  _result.get();
  for (auto& k_i : _up_list) {
    k_i->_result.get();
  }
}
trans_files::trans_files(rpc_file_system_client* in_self)
    : trans_file(in_self),
      _list() {
}
void trans_files::wait() {
  _result.wait();
  for (auto& k_i : _list) {
    k_i->wait();
  }
}
void trans_files::run() {
  for (auto& k_i : _list) {
    (*k_i)();
  }
}
}  // namespace rpc_trans

}  // namespace doodle
