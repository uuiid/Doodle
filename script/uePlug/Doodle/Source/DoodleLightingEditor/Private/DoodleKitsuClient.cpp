#include "DoodleKitsuClient.h"

#include "DoodleLightingJson.h"
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
	constexpr double RequestTimeoutSeconds = 30.0;

	struct FDoodleRequestState
	{
		FHttpRequestPtr Request;
		TValueOrError<TSharedPtr<FJsonObject>, FString> Body = MakeError(TEXT("请求没有等到响应"));
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

	TValueOrError<TSharedPtr<FJsonObject>, FString> MakeResponseBody(
		const FHttpResponsePtr& InResponse, bool bInConnectedSuccessfully, const FString& InUrl, bool bInSuccessCode
	)
	{
		if (!bInConnectedSuccessfully || !InResponse.IsValid())
		{
			return MakeError(FString::Printf(TEXT("无法连接服务器: %s"), *InUrl));
		}
		if (!bInSuccessCode)
		{
			return MakeError(DescribeFailure(InResponse->GetResponseCode(), InResponse->GetContentAsString()));
		}
		TSharedPtr<FJsonObject> L_Root;
		const TSharedRef<TJsonReader<>> L_Reader = TJsonReaderFactory<>::Create(InResponse->GetContentAsString());
		if (!FJsonSerializer::Deserialize(L_Reader, L_Root) || !L_Root.IsValid())
		{
			return MakeError(TEXT("服务器响应不是合法 json"));
		}
		return MakeValue(MoveTemp(L_Root));
	}

	FString MakeApiUrl(const FString& InServerUrl, const TCHAR* InPath)
	{
		return FString::Printf(TEXT("%s%s"), *InServerUrl, InPath);
	}

	void SubmitRequest(
		const TSharedRef<FDoodleRequestState>& InState, const FString& InVerb, const FString& InUrl,
		const FString& InBodyText, TFunction<bool(int32)> InIsSuccessCode, TFunction<void()> InOnDone
	)
	{
		InState->Request = FHttpModule::Get().CreateRequest();
		InState->Request->SetURL(InUrl);
		InState->Request->SetVerb(InVerb);
		InState->Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
		InState->Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
		InState->Request->SetTimeout(RequestTimeoutSeconds);
		InState->Request->SetContentAsString(InBodyText);
		InState->Request->SetDelegateThreadPolicy(EHttpRequestDelegateThreadPolicy::CompleteOnGameThread);

		InState->Request->OnProcessRequestComplete().BindLambda(
			[InState, InUrl, L_IsSuccessCode = MoveTemp(InIsSuccessCode)](
				FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bInConnectedSuccessfully
			)
			{
				const int32 L_Code = InResponse.IsValid() ? InResponse->GetResponseCode() : 0;
				InState->Body =
					MakeResponseBody(InResponse, bInConnectedSuccessfully, InUrl, bInConnectedSuccessfully && L_IsSuccessCode(L_Code));
				InState->bCompleted = true;
			}
		);

		const double L_Deadline = FPlatformTime::Seconds() + RequestTimeoutSeconds;
		FTSTicker::GetCoreTicker().AddTicker(
			TEXT("DoodleKitsuRequest"), 0.0f,
			[InState, InUrl, L_Deadline, L_OnDone = MoveTemp(InOnDone)](float InDeltaTime) mutable
			{
				if (!InState->bCompleted)
				{
					if (FPlatformTime::Seconds() < L_Deadline)
					{
						return true;
					}
					if (InState->Request.IsValid())
					{
						InState->Request->CancelRequest();
					}
					InState->Body = MakeError(FString::Printf(TEXT("请求超时: %s"), *InUrl));					InState->bCompleted = true;
				}
				L_OnDone();
				return false;
			}
		);
	}

	DoodleKitsuClient::FDoodleLoginValue ParseLoginBody(const TSharedPtr<FJsonObject>& InRoot)
	{
		DoodleKitsuClient::FDoodleLoginValue L_Value;

		FString L_AccessToken;
		if (!InRoot->TryGetStringField(TEXT("access_token"), L_AccessToken) || L_AccessToken.IsEmpty())
		{
			throw FString(TEXT("登录响应里没有 access_token"));
		}

		FString L_RefreshToken;
		InRoot->TryGetStringField(TEXT("refresh_token"), L_RefreshToken);

		const TSharedPtr<FJsonObject>* L_User = nullptr;
		if (InRoot->TryGetObjectField(TEXT("user"), L_User) && L_User != nullptr && L_User->IsValid())
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

		return L_Value;
	}

	DoodleKitsuClient::FDoodleLoginResult MakeLoginResult(
		const TValueOrError<TSharedPtr<FJsonObject>, FString>& InBody, const FString& InEmailBeforeLogin
	)
	{
		if (InBody.HasError())
		{
			GetSettings()->Email = InEmailBeforeLogin;
			return MakeError(InBody.GetError());
		}
		try
		{
			return MakeValue(ParseLoginBody(InBody.GetValue()));
		}
		catch (const FString& L_Error)
		{
			GetSettings()->Email = InEmailBeforeLogin;
			return MakeError(L_Error);
		}
	}

	DoodleKitsuClient::FDoodleLightingIterationValue ParseIterationBody(const TSharedPtr<FJsonObject>& InRoot)
	{
		DoodleKitsuClient::FDoodleLightingIterationValue L_Value;

		if (!InRoot->TryGetNumberField(TEXT("iteration"), L_Value.Iteration))
		{
			throw FString(TEXT("轮次响应里没有 iteration"));
		}
		if (!InRoot->TryGetStringField(TEXT("state"), L_Value.State) || L_Value.State.IsEmpty())
		{
			throw FString(TEXT("轮次响应里没有 state"));
		}
		InRoot->TryGetStringField(TEXT("render_at"), L_Value.RenderAt);
		InRoot->TryGetStringField(TEXT("feedback_text"), L_Value.FeedbackText);

		const TSharedPtr<FJsonObject>* L_RigSet = nullptr;
		if (!InRoot->TryGetObjectField(TEXT("rigs"), L_RigSet) || L_RigSet == nullptr || !L_RigSet->IsValid())
		{
			return L_Value;
		}

		(*L_RigSet)->TryGetStringField(TEXT("session_id"), L_Value.SessionId);
		(*L_RigSet)->TryGetStringField(TEXT("shot_task_id"), L_Value.ShotTaskId);
		(*L_RigSet)->TryGetStringField(TEXT("baseline_level"), L_Value.BaselineLevel);

		const TArray<TSharedPtr<FJsonValue>>* L_Rigs = nullptr;
		if (!(*L_RigSet)->TryGetArrayField(TEXT("rigs"), L_Rigs) || L_Rigs == nullptr)
		{
			return L_Value;
		}

		for (int32 L_Index = 0; L_Index < L_Rigs->Num(); ++L_Index)
		{
			const TSharedPtr<FJsonObject>* L_Item = nullptr;
			if (!(*L_Rigs)[L_Index].IsValid() || !(*L_Rigs)[L_Index]->TryGetObject(L_Item) || L_Item == nullptr ||
				!L_Item->IsValid())
			{
				throw FString::Printf(TEXT("rigs.rigs[%d]：不是对象"), L_Index);
			}
			FDoodleLightingRig L_Rig;
			FString L_Error;
			if (!DoodleLightingJson::ParseLightingRig(*L_Item, L_Rig, L_Error))
			{
				throw FString::Printf(TEXT("rigs.rigs[%d]：%s"), L_Index, *L_Error);
			}
			L_Value.Rigs.Add(MoveTemp(L_Rig));
		}

		return L_Value;
	}

	DoodleKitsuClient::FDoodleIterationResult MakeIterationResult(
		const TValueOrError<TSharedPtr<FJsonObject>, FString>& InBody
	)
	{
		if (InBody.HasError())
		{
			return MakeError(InBody.GetError());
		}
		try
		{
			return MakeValue(ParseIterationBody(InBody.GetValue()));
		}
		catch (const FString& L_Error)
		{
			return MakeError(L_Error);
		}
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
		const FString L_Url = MakeApiUrl(L_ServerUrl, TEXT("/api/auth/login"));
		TSharedRef<FDoodleRequestState> L_State = MakeShared<FDoodleRequestState>();
		SubmitRequest(
			L_State, TEXT("POST"), L_Url, L_BodyText,
			[](int32 InStatusCode) { return InStatusCode == EHttpResponseCodes::Created; },
			[L_State, L_EmailBeforeLogin, L_OnComplete = MoveTemp(InOnComplete)]() mutable
			{
				FDoodleLoginResult L_Result = MakeLoginResult(L_State->Body, L_EmailBeforeLogin);
				if (L_Result.HasValue())
				{
					UE_LOG(LogTemp, Log, TEXT("[DoodleKitsu] 登录成功 %s"), *L_Result.GetValue().Email);
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("[DoodleKitsu] 登录失败 %s"), *L_Result.GetError());
				}
				L_OnComplete(MoveTemp(L_Result));
			}
		);

		return MakeValue(FDoodleLoginValue{});
	}

	FDoodleIterationResult StartFetchCurrentIteration(const FString& InSessionId, FDoodleIterationCallback InOnComplete)
	{
		FString L_SessionId = InSessionId;
		L_SessionId.TrimStartAndEndInline();
		if (L_SessionId.IsEmpty())
		{
			return MakeError(TEXT("会话 ID 为空"));
		}

		if (!InOnComplete)
		{
			return MakeError(TEXT("轮次回调为空"));
		}

		if (!IsLoggedIn())
		{
			return MakeError(TEXT("未登录，无法获取会话轮次"));
		}

		const FString L_ServerUrl = GetServerUrl();
		if (L_ServerUrl.IsEmpty())
		{
			return MakeError(TEXT("未设置服务器地址（Project Settings -> Plugins -> Doodle Lighting）"));
		}

		const FString L_Url = FString::Printf(TEXT("%s/api/doodle/ai/light/session/%s/iteration"), *L_ServerUrl, *L_SessionId);

		TSharedRef<FDoodleRequestState> L_State = MakeShared<FDoodleRequestState>();
		SubmitRequest(
			L_State, TEXT("GET"), L_Url, FString(),
			[](int32 InStatusCode) { return EHttpResponseCodes::IsOk(InStatusCode); },
			[L_State, L_OnComplete = MoveTemp(InOnComplete)]() mutable
			{
				FDoodleIterationResult L_Result = MakeIterationResult(L_State->Body);
				if (L_Result.HasValue())
				{
					UE_LOG(
						LogTemp, Log, TEXT("[DoodleKitsu] 获取第 %d 轮成功（%d 个候选）"), L_Result.GetValue().Iteration,
						L_Result.GetValue().Rigs.Num()
					);
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("[DoodleKitsu] 获取轮次失败 %s"), *L_Result.GetError());
				}
				L_OnComplete(MoveTemp(L_Result));
			}
		);

		return MakeValue(FDoodleLightingIterationValue{});
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
