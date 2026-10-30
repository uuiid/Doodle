#include "DoodleKitsuClient.h"

#include "DoodleLightingSettings.h"
#include "DoodleListenerPortShm.h"
#include "DoodleLightingJson.h"
#include "DoodleWebCookie.h"

#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	constexpr double RequestTimeoutSeconds = 30.0;
	const TCHAR* const LocalIterationPath = TEXT("/api/actions/local/ai_session/ue");
	const TCHAR* const LocalLoginPath = TEXT("/api/auth/login");
	const TCHAR* const LocalBearerToken = TEXT("xxxxx");

	struct FDoodleRequestState
	{
		FHttpRequestPtr Request;
		TValueOrError<TSharedPtr<FJsonObject>, FString> Body = MakeError(TEXT("请求没有等到响应"));
		bool bCompleted = false;
	};

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
			return FString::Printf(TEXT("%d 请求失败"), InStatusCode);
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

	void SubmitRequest(
		const TSharedRef<FDoodleRequestState>& InState, const FString& InVerb, const FString& InUrl,
		const FString& InAuthorization, TFunction<bool(int32)> InIsSuccessCode, TFunction<void()> InOnDone,
		const FString& InBody = FString()
	)
	{
		InState->Request = FHttpModule::Get().CreateRequest();
		InState->Request->SetURL(InUrl);
		InState->Request->SetVerb(InVerb);
		InState->Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
		InState->Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
		if (!InAuthorization.IsEmpty())
		{
			InState->Request->SetHeader(TEXT("Authorization"), InAuthorization);
		}
		if (!InBody.IsEmpty())
		{
			InState->Request->SetContentAsString(InBody);
		}
		InState->Request->SetTimeout(RequestTimeoutSeconds);
		InState->Request->SetDelegateThreadPolicy(EHttpRequestDelegateThreadPolicy::CompleteOnGameThread);

		InState->Request->OnProcessRequestComplete().BindLambda(
			[InState, InUrl, L_IsSuccessCode = MoveTemp(InIsSuccessCode)](
				FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bInConnectedSuccessfully
			)
			{
				const int32 L_Code = InResponse.IsValid() ? InResponse->GetResponseCode() : 0;
				InState->Body = MakeResponseBody(
					InResponse, bInConnectedSuccessfully, InUrl, bInConnectedSuccessfully && L_IsSuccessCode(L_Code)
				);
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
					InState->Body = MakeError(FString::Printf(TEXT("请求超时: %s"), *InUrl));
					InState->bCompleted = true;
				}
				L_OnDone();
				return false;
			}
		);
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

	DoodleKitsuClient::FDoodleLoginResult MakeLoginResult(
		const TValueOrError<TSharedPtr<FJsonObject>, FString>& InBody, const FString& InFallbackEmail
	)
	{
		if (InBody.HasError())
		{
			return MakeError(InBody.GetError());
		}

		const TSharedPtr<FJsonObject>& L_Root = InBody.GetValue();

		DoodleKitsuClient::FDoodleLoginValue L_Value;
		if (!L_Root->TryGetStringField(TEXT("access_token"), L_Value.AccessToken) || L_Value.AccessToken.IsEmpty())
		{
			return MakeError(TEXT("登录响应里没有 access_token"));
		}
		L_Root->TryGetStringField(TEXT("refresh_token"), L_Value.RefreshToken);

		const TSharedPtr<FJsonObject>* L_User = nullptr;
		if (L_Root->TryGetObjectField(TEXT("user"), L_User) && L_User != nullptr && L_User->IsValid())
		{
			(*L_User)->TryGetStringField(TEXT("email"), L_Value.Email);
			(*L_User)->TryGetStringField(TEXT("full_name"), L_Value.DisplayName);
			if (L_Value.DisplayName.IsEmpty())
			{
				(*L_User)->TryGetStringField(TEXT("name"), L_Value.DisplayName);
			}
		}
		if (L_Value.Email.IsEmpty())
		{
			L_Value.Email = InFallbackEmail;
		}

		return MakeValue(MoveTemp(L_Value));
	}

	FString ResolveLoginUrl(const FString& InServerUrl, FString& OutError)
	{
		const FString L_Url = InServerUrl.TrimStartAndEnd();
		if (L_Url.IsEmpty())
		{
			OutError = TEXT("远程 URL 为空");
			return FString();
		}
		if (!L_Url.StartsWith(TEXT("http://")) && !L_Url.StartsWith(TEXT("https://")))
		{
			OutError = TEXT("远程 URL 必须以 http:// 或 https:// 开头");
			return FString();
		}
		return FString::Printf(TEXT("%s%s"), *L_Url.TrimEnd(), LocalLoginPath);
	}
}

namespace DoodleKitsuClient
{
	TValueOrError<FString, FString> GetLocalServerUrl()
	{
		const FDoodleListenerPortResult L_Port = ReadListenerPort();
		if (L_Port.HasError())
		{
			return MakeError(L_Port.GetError());
		}
		return MakeValue(FString::Printf(TEXT("http://127.0.0.1:%u"), L_Port.GetValue().Port));
	}

	FDoodleIterationResult StartFetchCurrentIteration(FDoodleIterationCallback InOnComplete)
	{
		if (!InOnComplete)
		{
			return MakeError(TEXT("轮次回调为空"));
		}

		const TValueOrError<FString, FString> L_ServerUrl = GetLocalServerUrl();
		if (L_ServerUrl.HasError())
		{
			return MakeError(FString::Printf(TEXT("读不到本机服务端口: %s"), *L_ServerUrl.GetError()));
		}

		const FString L_Url = FString::Printf(TEXT("%s%s"), *L_ServerUrl.GetValue(), LocalIterationPath);
		TSharedRef<FDoodleRequestState> L_State = MakeShared<FDoodleRequestState>();
		SubmitRequest(
			L_State, TEXT("GET"), L_Url, FString::Printf(TEXT("Bearer %s"), LocalBearerToken),
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

	FDoodleLoginResult StartLogin(
		const FString& InServerUrl, const FString& InEmail, const FString& InPassword, FDoodleLoginCallback InOnComplete
	)
	{
		if (!InOnComplete)
		{
			return MakeError(TEXT("登录回调为空"));
		}
		if (InEmail.TrimStartAndEnd().IsEmpty())
		{
			return MakeError(TEXT("邮箱为空"));
		}
		if (InPassword.IsEmpty())
		{
			return MakeError(TEXT("密码为空"));
		}

		FString L_UrlError;
		const FString L_Url = ResolveLoginUrl(InServerUrl, L_UrlError);
		if (!L_UrlError.IsEmpty())
		{
			return MakeError(L_UrlError);
		}

		const TSharedRef<FJsonObject> L_RequestRoot = MakeShared<FJsonObject>();
		L_RequestRoot->SetStringField(TEXT("email"), InEmail.TrimStartAndEnd());
		L_RequestRoot->SetStringField(TEXT("password"), InPassword);
		L_RequestRoot->SetBoolField(TEXT("isTrusted"), true);

		FString L_RequestBody;
		const TSharedRef<TJsonWriter<>> L_Writer = TJsonWriterFactory<>::Create(&L_RequestBody);
		if (!FJsonSerializer::Serialize(L_RequestRoot, L_Writer))
		{
			return MakeError(TEXT("登录请求体序列化失败"));
		}

		const FString L_Email = InEmail.TrimStartAndEnd();
		const TValueOrError<FString, FString> L_Origin = DoodleWebCookie::ResolveCookieOrigin(InServerUrl);
		if (L_Origin.HasError())
		{
			return MakeError(L_Origin.GetError());
		}

		TSharedRef<FDoodleRequestState> L_State = MakeShared<FDoodleRequestState>();
		SubmitRequest(
			L_State, TEXT("POST"), L_Url, FString(),
			[](int32 InStatusCode) { return EHttpResponseCodes::IsOk(InStatusCode); },
			[L_State, L_Email, L_CookieOrigin = L_Origin.GetValue(), L_OnComplete = MoveTemp(InOnComplete)]() mutable
			{
				FDoodleLoginResult L_Result = MakeLoginResult(L_State->Body, L_Email);
				if (L_Result.HasError())
				{
					UE_LOG(LogTemp, Error, TEXT("[DoodleKitsu] 登录失败 %s"), *L_Result.GetError());
					L_OnComplete(MoveTemp(L_Result));
					return;
				}

				const FString L_AccessToken = L_Result.GetValue().AccessToken;
				UE_LOG(LogTemp, Log, TEXT("[DoodleKitsu] 登录成功 %s"), *L_Email);

				DoodleWebCookie::SetAccessTokenCookie(
					L_CookieOrigin, L_AccessToken,
					[L_AccessToken, L_Result = MoveTemp(L_Result), L_OnComplete = MoveTemp(L_OnComplete)](
						bool bInWritten
					) mutable
					{
						if (!bInWritten)
						{
							L_OnComplete(MakeError(TEXT("登录成功，但没能把 cookie 写进浏览器")));
							return;
						}

						UDoodleLightingSettings* L_Settings = GetMutableDefault<UDoodleLightingSettings>();
						L_Settings->Cookies = L_AccessToken;
						L_Settings->SaveConfig();

						L_OnComplete(MoveTemp(L_Result));
					}
				);
			},
			L_RequestBody
		);

		return MakeValue(FDoodleLoginValue{});
	}
}
