#include <Doodle/MovieRemoteExecutor.h>

THIRD_PARTY_INCLUDES_START
#include "Windows.h"

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
