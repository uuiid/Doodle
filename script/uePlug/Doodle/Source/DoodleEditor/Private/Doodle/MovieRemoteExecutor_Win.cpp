#include <Doodle/MovieRemoteExecutor.h>

THIRD_PARTY_INCLUDES_START
// NOMINMAX: 这个文件所在的 unity 分组后面还有别的 .cpp, windows.h 的 min/max 宏会泄漏过去,
// 把引擎头 (RHI 等) 里的 std::numeric_limits<T>::max() 打坏。必须在 include 之前定义。
#ifndef NOMINMAX
	#define NOMINMAX
#endif
#include "Windows.h"

// winnt.h 还会把这些名字定义成宏 (InterlockedIncrement -> _InterlockedIncrement 之类),
// 同样会泄漏到后面的引擎头里, 让 FPlatformAtomics::InterlockedIncrement 变成不存在的成员
// (IoBuffer.h 就是这么炸的)。本文件只用到注册表 API, 这里全部撤掉。
#undef InterlockedIncrement
#undef InterlockedDecrement
#undef InterlockedExchange
#undef InterlockedExchangeAdd
#undef InterlockedCompareExchange
#undef InterlockedAnd
#undef InterlockedOr
#undef InterlockedXor
#undef InterlockedAdd
#undef InterlockedIncrement64
#undef InterlockedDecrement64
#undef InterlockedExchange64
#undef InterlockedExchangeAdd64
#undef InterlockedCompareExchange64
#undef InterlockedAnd64
#undef InterlockedOr64
#undef InterlockedXor64
#undef InterlockedAdd64
#undef InterlockedIncrement16
#undef InterlockedDecrement16
#undef InterlockedExchange16
#undef InterlockedCompareExchange16
#undef InterlockedExchange8
#undef InterlockedExchangeAdd8
#undef InterlockedAnd8
#undef InterlockedOr8
#undef InterlockedXor8
#undef InterlockedAnd16
#undef InterlockedOr16
#undef InterlockedXor16
#undef InterlockedExchangePointer
#undef InterlockedCompareExchangePointer

void UDoodleMovieRemoteExecutor::FindRegServerAddress() {
  HKEY L_Key{nullptr};

  LSTATUS L_RetCode =
      ::RegOpenKeyExW(HKEY_CURRENT_USER, TEXT("Software\\Doodle\\RenderFarm"), REG_NONE, KEY_READ, &L_Key);

  if (L_RetCode != ERROR_SUCCESS) {
    UE_LOG(LogTemp, Log, TEXT("RemoteClientRender Reg Erroe"), L_RetCode);
    return;
  }
  DWORD datasize = 0;
  L_RetCode      = ::RegGetValueW(L_Key, nullptr, TEXT("server_address"), RRF_RT_REG_SZ, nullptr, nullptr, &datasize);
  if (L_RetCode != ERROR_SUCCESS) {
    UE_LOG(LogTemp, Log, TEXT("RemoteClientRender Reg Erroe"), L_RetCode);

    return;
  }
  FString L_ServerAddress{int32(datasize / sizeof(wchar_t)), TEXT(" ")};

  L_RetCode = ::RegGetValueW(
      L_Key, nullptr, TEXT("server_address"), RRF_RT_REG_SZ, nullptr, L_ServerAddress.GetCharArray().GetData(),
      &datasize
  );
  if (L_RetCode != ERROR_SUCCESS) {
    UE_LOG(LogTemp, Log, TEXT("RemoteClientRender Reg Erroe"), L_RetCode);
    return;
  }
  Remote_Server_Ip = L_ServerAddress;
  FindRemoteClient();
}

THIRD_PARTY_INCLUDES_END
