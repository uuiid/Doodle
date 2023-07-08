//
// Created by td_main on 2023/7/8.
//

#include "cloud_fetch_data.h"

namespace doodle::detail {
void cloud_fetch_data::init() {
  stream_handle_.assign(::CreateFileW(
      server_path_.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr
  ));
}

void cloud_fetch_data::async_read() {
  boost::asio::async_read_at(
      stream_handle_, start_offset_, boost::asio::buffer(buffer_.get(), length_),
      [this, l_s = shared_from_this()](const boost::system::error_code& ec, std::size_t bytes_transferred) {
        if (ec && ec != boost::asio::error::eof) {
          // 读取失败
          DOODLE_LOG_INFO(
              "[{}:{}] - Async read failed for {}, Status {}\n", GetCurrentProcessId(), GetCurrentThreadId(),
              server_path_, ec.what()
          );
          return;
        }
        // 读取成功
        DOODLE_LOG_INFO(
            "[{}:{}] - Async read success for {}, Status {}\n", GetCurrentProcessId(), GetCurrentThreadId(),
            server_path_, ec.what()
        );

        transfer_data(
            callback_info_.ConnectionKey, callback_info_.TransferKey, buffer_.get(), start_offset_, bytes_transferred,
            STATUS_SUCCESS
        );
        STATUS_CLOUD_FILE_IN_USE;
        start_offset_ += bytes_transferred;
        remaining_length_.QuadPart -= bytes_transferred;
        if (remaining_length_.QuadPart > 0) {
          async_read();
        }
      }
  );
}

void cloud_fetch_data::transfer_data(
    int connectionKey, LARGE_INTEGER transferKey, LPCVOID transferData, std::size_t startingOffset, std::size_t length,
    NTSTATUS completionStatus
) {
  CF_OPERATION_INFO opInfo               = {0};
  CF_OPERATION_PARAMETERS opParams       = {0};

  opInfo.StructSize                      = sizeof(opInfo);
  opInfo.Type                            = CF_OPERATION_TYPE_TRANSFER_DATA;
  opInfo.ConnectionKey                   = connectionKey;
  opInfo.TransferKey                     = transferKey;
  opInfo.RequestKey                      = callback_info_.RequestKey;

  opParams.ParamSize                     = RTL_SIZEOF_THROUGH_FIELD(CF_OPERATION_PARAMETERS, TransferData);
  opParams.TransferData.CompletionStatus = completionStatus;
  opParams.TransferData.Flags            = CF_OPERATION_TRANSFER_DATA_FLAG_NONE;
  opParams.TransferData.Buffer           = transferData;
  opParams.TransferData.Offset           = cloud_provider_registrar::longlong_to_large_integer(startingOffset);
  opParams.TransferData.Length           = cloud_provider_registrar::longlong_to_large_integer(length);

  LOG_IF_FAILED(::CfExecute(&opInfo, &opParams));
}

}  // namespace doodle::detail