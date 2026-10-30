#include "DoodleListenerPortShm.h"

#include "Windows/WindowsHWrapper.h"

namespace
{
	constexpr uint32 GShmMagic = 0x444C5054U;
	constexpr uint32 GShmVersion = 1;

	struct FShmData
	{
		uint32 Magic = 0;
		uint32 Version = 0;
		uint16 Port = 0;
		uint16 Reserved = 0;
	};

	static_assert(sizeof(FShmData) == 12, "共享内存结构与后端 http_listener_port_shm_t 必须逐字节一致");

	struct FHandleGuard
	{
		HANDLE Handle = nullptr;
		~FHandleGuard()
		{
			if (Handle != nullptr)
			{
				::CloseHandle(Handle);
			}
		}
	};

	struct FMapGuard
	{
		const void* Address = nullptr;
		~FMapGuard()
		{
			if (Address != nullptr)
			{
				::UnmapViewOfFile(Address);
			}
		}
	};

	DoodleKitsuClient::FDoodleListenerPortResult MakeSharedMemoryError(const FString& InName, const FString& InReason)
	{
		return MakeError(FString::Printf(TEXT("共享内存 %s %s"), *InName, *InReason));
	}
}

namespace DoodleKitsuClient
{
	FString GetListenerPortShmName()
	{
		return TEXT("doodle_http_listener_port");
	}

	FDoodleListenerPortResult ReadListenerPort()
	{
		const FString L_Name = GetListenerPortShmName();
		const FString L_LocalName = FString::Printf(TEXT("Local\\%s"), *L_Name);

		FHandleGuard L_Handle;
		L_Handle.Handle = ::OpenFileMappingW(FILE_MAP_READ, 0, *L_LocalName);
		if (L_Handle.Handle == nullptr)
		{
			L_Handle.Handle = ::OpenFileMappingW(FILE_MAP_READ, 0, *L_Name);
		}
		if (L_Handle.Handle == nullptr)
		{
			return MakeError(
				FString::Printf(TEXT("共享内存 %s 不存在或打不开（错误码 %u）"), *L_Name, ::GetLastError())
			);
		}

		FMapGuard L_Map;
		L_Map.Address = ::MapViewOfFile(L_Handle.Handle, FILE_MAP_READ, 0, 0, 0);
		if (L_Map.Address == nullptr)
		{
			return MakeSharedMemoryError(L_Name, TEXT("无法映射到本进程（错误码 ") + FString::FromInt(::GetLastError()) + TEXT("）"));
		}

		MEMORY_BASIC_INFORMATION L_Region{};
		if (::VirtualQuery(L_Map.Address, &L_Region, sizeof(L_Region)) == 0)
		{
			return MakeSharedMemoryError(L_Name, TEXT("无法查询尺寸"));
		}
		if (L_Region.RegionSize < sizeof(FShmData))
		{
			return MakeSharedMemoryError(L_Name, TEXT("尺寸不足"));
		}

		FShmData L_Data;
		FMemory::Memcpy(&L_Data, L_Map.Address, sizeof(L_Data));

		if (L_Data.Magic != GShmMagic)
		{
			return MakeSharedMemoryError(L_Name, TEXT("里的魔数不匹配"));
		}
		if (L_Data.Version != GShmVersion)
		{
			return MakeError(
				FString::Printf(TEXT("共享内存版本 %u 不认识（期望 %u）"), L_Data.Version, GShmVersion)
			);
		}

		return MakeValue(FDoodleListenerPortValue{L_Data.Port});
	}
}
