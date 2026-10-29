#include "DoodleKitsuClient.h"

#include "DoodleLightingSettings.h"

#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	constexpr double LoginTimeoutSeconds = 30.0;

	struct FLoginRequestState
	{
		FHttpRequestPtr Request;
		DoodleKitsuClient::FDoodleLoginResult Result = MakeError(TEXT("登录请求没有等到响应"));
		bool bCompleted = false;
	};

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
		TSharedPtr<FJsonObject> L_Root;
		const TSharedRef<TJsonReader<>> L_Reader = TJsonReaderFactory<>::Create(InBody);
		if (!FJsonSerializer::Deserialize(L_Reader, L_Root) || !L_Root.IsValid())
		{
			return MakeError(TEXT("登录响应不是合法 json"));
		}

		FString L_AccessToken;
		if (!L_Root->TryGetStringField(TEXT("access_token"), L_AccessToken) || L_AccessToken.IsEmpty())
		{
			return MakeError(TEXT("登录响应里没有 access_token"));
		}

		DoodleKitsuClient::FDoodleLoginValue L_Value;

		FString L_RefreshToken;
		L_Root->TryGetStringField(TEXT("refresh_token"), L_RefreshToken);

		const TSharedPtr<FJsonObject>* L_User = nullptr;
		if (L_Root->TryGetObjectField(TEXT("user"), L_User) && L_User != nullptr && L_User->IsValid())
		{
			(*L_User)->TryGetStringField(TEXT("id"), L_Value.UserId);
			if (!(*L_User)->TryGetStringField(TEXT("full_name"), L_Value.UserName) || L_Value.UserName.IsEmpty())
			{
				(*L_User)->TryGetStringField(TEXT("first_name"), L_Value.UserName);
			}
			(*L_User)->TryGetStringField(TEXT("email"), L_Value.Email);
		}

		UDoodleLightingSettings* L_Settings = GetSettings();
		L_Settings->AccessToken = L_AccessToken;
		L_Settings->RefreshToken = L_RefreshToken;
		L_Settings->UserId = L_Value.UserId;
		L_Settings->UserName = L_Value.UserName;
		if (!L_Value.Email.IsEmpty())
		{
			L_Settings->Email = L_Value.Email;
		}
		L_Settings->TryUpdateDefaultConfigFile();

		return MakeValue(MoveTemp(L_Value));
	}

	DoodleKitsuClient::FDoodleLoginResult MakeResponseResult(
		FHttpResponsePtr InResponse, bool bInConnectedSuccessfully, const FString& InServerUrl
	)
	{
		if (!bInConnectedSuccessfully || !InResponse.IsValid())
		{
			return MakeError(FString::Printf(TEXT("无法连接服务器: %s"), *InServerUrl));
		}
		if (InResponse->GetResponseCode() == EHttpResponseCodes::Created)
		{
			return ParseLoginResponse(InResponse->GetContentAsString());
		}
		return MakeError(DescribeFailure(InResponse->GetResponseCode(), InResponse->GetContentAsString()));
	}

	void ReportLoginResult(
		const TSharedRef<FLoginRequestState>& InState, const FString& InEmailBeforeLogin,
		const DoodleKitsuClient::FDoodleLoginCallback& InOnComplete
	)
	{
		if (InState->Result.HasValue())
		{
			UE_LOG(LogTemp, Log, TEXT("[DoodleKitsu] 登录成功 %s"), *InState->Result.GetValue().Email);
		}
		else
		{
			GetSettings()->Email = InEmailBeforeLogin;
			UE_LOG(LogTemp, Error, TEXT("[DoodleKitsu] 登录失败 %s"), *InState->Result.GetError());
		}
		InOnComplete(MoveTemp(InState->Result));
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

	FDoodleLoginResult StartLogin(const FString& InEmail, const FString& InPassword, FDoodleLoginCallback InOnComplete)
	{
		FString L_Email = InEmail;
		L_Email.TrimStartAndEndInline();
		if (L_Email.IsEmpty() || InPassword.IsEmpty())
		{
			return MakeError(TEXT("email 或 password 为空"));
		}

		if (!InOnComplete)
		{
			return MakeError(TEXT("登录回调为空"));
		}

		const FString L_ServerUrl = GetServerUrl();
		if (L_ServerUrl.IsEmpty())
		{
			return MakeError(TEXT("未设置服务器地址（Project Settings -> Plugins -> Doodle Lighting）"));
		}

		TSharedPtr<FJsonObject> L_Body = MakeShared<FJsonObject>();
		L_Body->SetStringField(TEXT("email"), L_Email);
		L_Body->SetStringField(TEXT("password"), InPassword);

		FString L_BodyText;
		const TSharedRef<TJsonWriter<>> L_Writer = TJsonWriterFactory<>::Create(&L_BodyText);
		if (!FJsonSerializer::Serialize(L_Body.ToSharedRef(), L_Writer))
		{
			return MakeError(TEXT("登录请求体序列化失败"));
		}

		const FString L_EmailBeforeLogin = GetSettings()->Email;

		TSharedRef<FLoginRequestState> L_State = MakeShared<FLoginRequestState>();
		L_State->Request = FHttpModule::Get().CreateRequest();
		L_State->Request->SetURL(FString::Printf(TEXT("%s/api/auth/login"), *L_ServerUrl));
		L_State->Request->SetVerb(TEXT("POST"));
		L_State->Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
		L_State->Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
		L_State->Request->SetTimeout(LoginTimeoutSeconds);
		L_State->Request->SetContentAsString(L_BodyText);
		L_State->Request->SetDelegateThreadPolicy(EHttpRequestDelegateThreadPolicy::CompleteOnGameThread);

		L_State->Request->OnProcessRequestComplete().BindLambda(
			[L_State, L_ServerUrl](
				FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bInConnectedSuccessfully
			)
			{
				L_State->Result = MakeResponseResult(InResponse, bInConnectedSuccessfully, L_ServerUrl);
				L_State->bCompleted = true;
			}
		);

		if (!L_State->Request->ProcessRequest())
		{
			GetSettings()->Email = L_EmailBeforeLogin;
			return MakeError(FString::Printf(TEXT("登录请求发不出去: %s"), *L_ServerUrl));
		}

		const double L_Deadline = FPlatformTime::Seconds() + LoginTimeoutSeconds;
		FTSTicker::GetCoreTicker().AddTicker(
			TEXT("DoodleKitsuLogin"), 0.0f,
			[L_State, L_ServerUrl, L_Deadline, L_EmailBeforeLogin,
			 L_OnComplete = MoveTemp(InOnComplete)](float InDeltaTime) mutable
			{
				if (!L_State->bCompleted)
				{
					if (FPlatformTime::Seconds() < L_Deadline)
					{
						return true;
					}
					if (L_State->Request.IsValid())
					{
						L_State->Request->CancelRequest();
					}
					L_State->Result = MakeError(FString::Printf(TEXT("登录超时: %s"), *L_ServerUrl));
					L_State->bCompleted = true;
				}
				ReportLoginResult(L_State, L_EmailBeforeLogin, L_OnComplete);
				return false;
			}
		);

		return MakeValue(FDoodleLoginValue{});
	}

	bool Logout()
	{
		UDoodleLightingSettings* L_Settings = GetSettings();
		L_Settings->AccessToken.Empty();
		L_Settings->RefreshToken.Empty();
		L_Settings->UserId.Empty();
		L_Settings->UserName.Empty();
		return L_Settings->TryUpdateDefaultConfigFile();
	}
}
