//
// Created by TD on 2021/6/9.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/generate/rpc/file_system_server.grpc.pb.h>
#include <doodle_lib/lib_warp/protobuf_warp.h>
#include <doodle_lib/rpc/rpc_trans_path.h>
#include <doodle_lib/thread_pool/thread_pool.h>

#include <optional>

namespace doodle {
namespace rpc_trans {
class down_file;
class down_dir;
class up_file;
class up_dir;
class trans_file;
class trans_files;
using trans_file_ptr = std::shared_ptr<trans_file>;

class DOODLELIB_API trans_file
    : public details::no_copy,
      public std::enable_shared_from_this<trans_file> {
  friend down_dir;
  friend up_dir;
  friend trans_files;

 protected:
  rpc_file_system_client* _self;
  std::unique_ptr<rpc_trans_path> _param;
  std::future<void> _result;
  std::size_t _size;

  std::mutex _mutex;

  virtual void run(const long_term_ptr& in_term) = 0;

 public:
  explicit trans_file(rpc_file_system_client* in_self);
  void set_parameter(std::unique_ptr<rpc_trans_path>& in_path);
  virtual long_term_ptr operator()();
  virtual void wait() = 0;
};

class DOODLELIB_API trans_files : public trans_file {
 public:
  explicit trans_files(rpc_file_system_client* in_self);
  std::vector<trans_file_ptr> _list;

  void wait() override;

  long_term_ptr operator()() override;
 protected:
  void run(const long_term_ptr& in_term) override;
};

class DOODLELIB_API down_file : public trans_file {
 public:
  explicit down_file(rpc_file_system_client* in_self);

 protected:
  void wait() override;
  void run(const long_term_ptr& in_term) override;
};
class DOODLELIB_API down_dir : public trans_file {
  std::vector<std::shared_ptr<down_file>> _down_list;

 public:
  explicit down_dir(rpc_file_system_client* in_self);

 protected:
  void wait() override;
  rpc_trans_path_ptr_list down(const std::unique_ptr<rpc_trans_path>& in_path);
  void run(const long_term_ptr& in_term) override;
};
class DOODLELIB_API up_file : public trans_file {
 public:
  explicit up_file(rpc_file_system_client* in_self);

 protected:
  void wait() override;
  void run(const long_term_ptr& in_term) override;
};
class DOODLELIB_API up_dir : public trans_file {
  std::vector<std::shared_ptr<up_file>> _up_list;

 public:
  explicit up_dir(rpc_file_system_client* in_self);

 protected:
  void wait() override;
  rpc_trans_path_ptr_list update(const std::unique_ptr<rpc_trans_path>& in_path);
  void run(const long_term_ptr& in_term) override;
};

}  // namespace rpc_trans

//class equal_fun {
// public:
//  bool operator()(const std::tuple<std::optional<bool>, std::optional<bool> >& arg) {
//    return;
//  };
//};
//class is_down_fun {
//};
/**
 * @brief 这个类在上传和下载时会`先比较文件，当比较成功后， 不一致才会上传和下载
 *
 */
class DOODLELIB_API rpc_file_system_client : public details::no_copy {
  friend rpc_trans::up_file;
  friend rpc_trans::down_file;
  friend rpc_trans::down_dir;
  friend rpc_trans::up_dir;

 public:
  using trans_file_ptr = std::shared_ptr<rpc_trans::trans_file>;
  using time_point = chrono::sys_time_pos;

 protected:
  std::unique_ptr<file_system_server::Stub> p_stub;
  std::recursive_mutex p_mutex;

  /**
   * 这个是用来比较文件的函数
   * @param in_local_path 本地路径
   * @param in_server_path 服务器路径
   * @return std::tuple<std::optional<bool>,  是否相等
   *                    std::optional<bool>,  是否需要下载
   *                    std::optional<bool>,  服务器文件是否存在
   *                    std::size_t,          文件大小
   *                    time                  服务器文件时间
   *                   >
   */
  std::tuple<std::optional<bool>, std::optional<bool>, bool, std::size_t,time_point> compare_file_is_down(const FSys::path& in_local_path, const FSys::path& in_server_path);
  std::string get_hash(const FSys::path& in_path);

 public:
  /**
   *@brief 这个时用来判读是否进行同步或者下载的函子别名
   * std::tuple<std::optional<bool>,  是否相等
   *            std::optional<bool> > 是否需要下载
   */
  using syn_fun    = std::function<bool(const std::tuple<std::optional<bool>, std::optional<bool>>& arg)>;

  explicit rpc_file_system_client(const std::shared_ptr<grpc::Channel>& in_channel);
  /**
   * @brief 获得远程服务器中的文件的基本信息
   *
   * @param path
   * @return std::tuple<
   *                   std::size_t,  文件大小
   *                   bool,         是存在
   *                   time_point,   最后写入时间
   *                   bool          是文件夹
   *                   >
   */
  std::tuple<std::size_t, bool, time_point, bool> get_info(const FSys::path& in_server_path);

  std::size_t get_size(const FSys::path& in_server_path);
  /**
   * @brief 判断是否是文件夹
   *
   * @param in_server_path 服务器路径
   * @return std::tuple<
   *                   bool,是存在
   *                   bool 是是文件夹
   *                   >
   */
  std::tuple<bool, bool> is_folder(const FSys::path& in_server_path);
  time_point get_timestamp(const FSys::path& in_server_path);
  bool is_exist(const FSys::path& in_server_path);
  /**
   * @brief 下载服务器中的文件
   *
   * @param in_local_path 本地文件路径
   * @param in_server_path 服务器中的文件路径
   * @return true 下载完成
   * @return false 下载失败
   */
  [[nodiscard]] trans_file_ptr download(const FSys::path& in_local_path, const FSys::path& in_server_path);
  [[nodiscard]] trans_file_ptr download(std::vector<std::unique_ptr<rpc_trans_path>>& in_vector);
  [[nodiscard]] trans_file_ptr download(std::unique_ptr<rpc_trans_path>& in_vector);
  /**
   * @brief 将文件上传到服务器中
   * @param in_local_path 本地路径
   * @param in_server_path 服务器路径
   * @param in_backup_path 上传时的备份路径， 如果上传文件夹， 备份文件夹也应该是文件夹， 文件则对应文件
   * @return 上传结果
   */
  [[nodiscard]] trans_file_ptr upload(const FSys::path& in_local_path, const FSys::path& in_server_path, const FSys::path& in_backup_path = {});
  [[nodiscard]] trans_file_ptr upload(std::vector<std::unique_ptr<rpc_trans_path>>& in_vector);
  [[nodiscard]] trans_file_ptr upload(std::unique_ptr<rpc_trans_path>& in_vector);
  //  void DownloadFile(const FSys::path& in_local_path, const FSys::path& in_server_path,const syn_fun& in_syn_fun );
  //  void UploadFile(const FSys::path& in_local_path, const FSys::path& in_server_path,const syn_fun& in_syn_fun );
};

}  // namespace doodle
