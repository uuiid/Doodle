#include "DoodleKitsuClient.h"

#include "DoodleLightingSettings.h"

#include "Dom/JsonObject.h"
#include "HttpManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	UDoodleLightingSettings* GetSettings()
	{
		return GetMutableDefault<UDoodleLightingSettings>();
	}

	FString NormalizeServerUrl(const FString& InServerUrl)
	{
		FString L_Url = InServerUrl;
		L_Url.TrimStartAndEndInline();
		while (L_Url.EndsWith(TEXT("/")))
		{
			L_Url.LeftChopInline(1);
		}
		return L_Url;
	}

	FString DescribeFailure(int32 InStatusCode, const FString& InBody)
	{
		FString L_Message = InBody;
		TSharedPtr<FJsonObject> L_Root;
		const TSharedRef<TJsonReader<>> L_Reader = TJsonReaderFactory<>::Create(InBody);
		if (FJsonSerializer::Deserialize(L_Reader, L_Root) && L_Root.IsValid())
		{
			FString L_JsonMessage;
			if (L_Root->TryGetStringField(TEXT("message"), L_JsonMessage) && !L_JsonMessage.IsEmpty())
			{
				L_Message = L_JsonMessage;
			}
			else if (L_Root->TryGetStringField(TEXT("error"), L_JsonMessage) && !L_JsonMessage.IsEmpty())
			{
				L_Message = L_JsonMessage;
			}
		}
		L_Message.TrimStartAndEndInline();
		if (L_Message.IsEmpty())
		{
			return FString::Printf(TEXT("%d 登录失败"), InStatusCode);
		}
		return FString::Printf(TEXT("%d %s"), InStatusCode, *L_Message);
	}

	DoodleKitsuClient::FDoodleLoginResult ParseLoginResponse(const FString& InBody)
	{
		DoodleKitsuClient::FDoodleLoginResult L_Result;

		TSharedPtr<FJsonObject> L_Root;
		const TSharedRef<TJsonReader<>> L_Reader = TJsonReaderFactory<>::Create(InBody);
		if (!FJsonSerializer::Deserialize(L_Reader, L_Root) || !L_Root.IsValid())
		{
			L_Result.Error = TEXT("登录响应不是合法 json");
			return L_Result;
		}

		FString L_AccessToken;
		if (!L_Root->TryGetStringField(TEXT("access_token"), L_AccessToken) || L_AccessToken.IsEmpty())
		{
			L_Result.Error = TEXT("登录响应里没有 access_token");
			return L_Result;
		}

		FString L_RefreshToken;
		L_Root->TryGetStringField(TEXT("refresh_token"), L_RefreshToken);

		const TSharedPtr<FJsonObject>* L_User = nullptr;
		if (L_Root->TryGetObjectField(TEXT("user"), L_User) && L_User != nullptr && L_User->IsValid())
		{
			(*L_User)->TryGetStringField(TEXT("id"), L_Result.UserId);
			if (!(*L_User)->TryGetStringField(TEXT("full_name"), L_Result.UserName) || L_Result.UserName.IsEmpty())
			{
				(*L_User)->TryGetStringField(TEXT("first_name"), L_Result.UserName);
			}
			(*L_User)->TryGetStringField(TEXT("email"), L_Result.Email);
		}

		UDoodleLightingSettings* L_Settings = GetSettings();
		L_Settings->AccessToken = L_AccessToken;
		L_Settings->RefreshToken = L_RefreshToken;
		L_Settings->UserId = L_Result.UserId;
		L_Settings->UserName = L_Result.UserName;
		if (!L_Result.Email.IsEmpty())
		{
			L_Settings->Email = L_Result.Email;
		}
		L_Settings->TryUpdateDefaultConfigFile();

		L_Result.bSucceeded = true;
		return L_Result;
	}
}

namespace DoodleKitsuClient
{
	FString GetServerUrl()
	{
		return NormalizeServerUrl(GetSettings()->ServerUrl);
	}

	bool IsLoggedIn()
	{
		return !GetSettings()->AccessToken.IsEmpty();
	}

	FString MakeBearerAuthorization()
	{
		const FString L_Token = GetSettings()->AccessToken;
		if (L_Token.IsEmpty())
		{
			return FString();
		}
		return FString::Printf(TEXT("Bearer %s"), *L_Token);
	}

	FDoodleLoginResult Login(const FString& InEmail, const FString& InPassword)
	{
		FString L_Email = InEmail;
		L_Email.TrimStartAndEndInline();
		if (L_Email.IsEmpty() || InPassword.IsEmpty())
		{
			FDoodleLoginResult L_Result;
			L_Result.Error = TEXT("email 或 password 为空");
			return L_Result;
		}

		const FString L_ServerUrl = GetServerUrl();
		if (L_ServerUrl.IsEmpty())
		{
			FDoodleLoginResult L_Result;
			L_Result.Error = TEXT("未设置服务器地址（Project Settings -> Plugins -> Doodle Lighting）");
			return L_Result;
		}

		TSharedPtr<FJsonObject> L_Body = MakeShared<FJsonObject>();
		L_Body->SetStringField(TEXT("email"), L_Email);
		L_Body->SetStringField(TEXT("password"), InPassword);

		FString L_BodyText;
		const TSharedRef<TJsonWriter<>> L_Writer = TJsonWriterFactory<>::Create(&L_BodyText);
		if (!FJsonSerializer::Serialize(L_Body.ToSharedRef(), L_Writer))
		{
			FDoodleLoginResult L_Result;
			L_Result.Error = TEXT("登录请求体序列化失败");
			return L_Result;
		}

		const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> L_Request = FHttpModule::Get().CreateRequest();
		L_Request->SetURL(FString::Printf(TEXT("%s/api/auth/login"), *L_ServerUrl));
		L_Request->SetVerb(TEXT("POST"));
		L_Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
		L_Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
		L_Request->SetTimeout(30.0f);
		L_Request->SetContentAsString(L_BodyText);

		const FString L_EmailBeforeLogin = GetSettings()->Email;

		FDoodleLoginResult L_Result;
		bool bCompleted = false;
		L_Request->OnProcessRequestComplete().BindLambda(
			[&L_Result, &bCompleted, L_ServerUrl](
				FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bInConnectedSuccessfully
			)
			{
				if (!bInConnectedSuccessfully || !InResponse.IsValid())
				{
					L_Result = FDoodleLoginResult();
					L_Result.Error = FString::Printf(TEXT("无法连接服务器: %s"), *L_ServerUrl);
				}
				else if (InResponse->GetResponseCode() == EHttpResponseCodes::Ok)
				{
					L_Result = ParseLoginResponse(InResponse->GetContentAsString());
				}
				else
				{
					L_Result = FDoodleLoginResult();
					L_Result.Error = DescribeFailure(InResponse->GetResponseCode(), InResponse->GetContentAsString());
				}
				bCompleted = true;
			}
		);

		if (!L_Request->ProcessRequest())
		{
			L_Result = FDoodleLoginResult();
			L_Result.Error = FString::Printf(TEXT("登录请求发不出去: %s"), *L_ServerUrl);
			GetSettings()->Email = L_EmailBeforeLogin;
			return L_Result;
		}

		const double L_Deadline = FPlatformTime::Seconds() + 30.0;
		while (!bCompleted && FPlatformTime::Seconds() < L_Deadline)
		{
			FHttpModule::Get().GetHttpManager().Tick(0.01f);
			FPlatformProcess::Sleep(0.01f);
		}

		if (!bCompleted)
		{
			L_Result = FDoodleLoginResult();
			L_Result.Error = FString::Printf(TEXT("登录超时: %s"), *L_ServerUrl);
			GetSettings()->Email = L_EmailBeforeLogin;
			return L_Result;
		}

		if (L_Result.bSucceeded)
		{
			UE_LOG(LogTemp, Log, TEXT("[DoodleKitsu] 登录成功 %s"), *L_Result.Email);
		}
		else
		{
			GetSettings()->Email = L_EmailBeforeLogin;
			UE_LOG(LogTemp, Error, TEXT("[DoodleKitsu] 登录失败 %s"), *L_Result.Error);
		}
		return L_Result;
	}

	void Logout()
	{
		UDoodleLightingSettings* L_Settings = GetSettings();
		L_Settings->AccessToken.Empty();
		L_Settings->RefreshToken.Empty();
		L_Settings->UserId.Empty();
		L_Settings->UserName.Empty();
		L_Settings->TryUpdateDefaultConfigFile();
	}

	bool LoginIntoActiveSession(const FString& InEmail, const FString& InPassword)
	{
		return Login(InEmail, InPassword).bSucceeded;
	}
}
