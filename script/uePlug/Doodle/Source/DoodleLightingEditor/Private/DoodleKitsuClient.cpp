#include "DoodleKitsuClient.h"

#include "DoodleLightingEditorLog.h"
#include "DoodleLightingJson.h"
#include "DoodleLightingSettings.h"
#include "DoodleWebCookie.h"

#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	constexpr double RequestTimeoutSeconds = 30.0;
	const TCHAR* const LocalLoginPath = TEXT("/api/auth/login");

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

		if (!InState->Request->ProcessRequest())
		{
			InState->Body = MakeError(FString::Printf(TEXT("请求没能发出去: %s"), *InUrl));
			InState->bCompleted = true;
		}

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

	DoodleKitsuClient::FDoodleLightingIterationValue ParseIterationRoot(const TSharedPtr<FJsonObject>& InRoot)
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
		(*L_RigSet)->TryGetStringField(TEXT("render_map"), L_Value.RenderMap);
		(*L_RigSet)->TryGetStringField(TEXT("movie_pipeline_config"), L_Value.MoviePipelineConfig);

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

	FString TrimServerUrl(const FString& InServerUrl)
	{
		FString L_Url = InServerUrl.TrimStartAndEnd();
		while (L_Url.EndsWith(TEXT("/")))
		{
			L_Url.LeftChopInline(1);
		}
		return L_Url;
	}

	TValueOrError<TArray<FString>, FString> ListPngFrames(const FString& InFramesDir)
	{
		if (InFramesDir.IsEmpty())
		{
			return MakeError(FString(TEXT("帧目录为空")));
		}

		TArray<FString> L_Files;
		IFileManager::Get().FindFiles(L_Files, *FPaths::Combine(InFramesDir, TEXT("*.png")), true, false);
		if (L_Files.IsEmpty())
		{
			return MakeError(FString::Printf(TEXT("帧目录 %s 里没有 png"), *InFramesDir));
		}

		L_Files.Sort();
		TArray<FString> L_Paths;
		L_Paths.Reserve(L_Files.Num());
		for (const FString& L_File : L_Files)
		{
			L_Paths.Add(FPaths::ConvertRelativePathToFull(FPaths::Combine(InFramesDir, L_File)));
		}
		return MakeValue(MoveTemp(L_Paths));
	}

	FString ResolveAuthorization()
	{
		const FString L_Token = GetDefault<UDoodleLightingSettings>()->Cookies;
		return L_Token.IsEmpty() ? FString() : FString::Printf(TEXT("Bearer %s"), *L_Token);
	}

	struct FUploadJsonOptions
	{
		FString Url;
		FString Authorization;
		FString Body;
	};

	TValueOrError<FString, FString> PostJsonSync(const FUploadJsonOptions& InOptions)
	{
		const FHttpRequestPtr L_Request = FHttpModule::Get().CreateRequest();
		L_Request->SetURL(InOptions.Url);
		L_Request->SetVerb(TEXT("POST"));
		L_Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
		L_Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
		if (!InOptions.Authorization.IsEmpty())
		{
			L_Request->SetHeader(TEXT("Authorization"), InOptions.Authorization);
		}
		L_Request->SetContentAsString(InOptions.Body);
		L_Request->SetTimeout(RequestTimeoutSeconds);

		L_Request->ProcessRequestUntilComplete();

		const FHttpResponsePtr L_Response = L_Request->GetResponse();
		if (!L_Response.IsValid())
		{
			return MakeError(FString::Printf(TEXT("无法连接服务器: %s"), *InOptions.Url));
		}
		const int32 L_Code = L_Response->GetResponseCode();
		if (!EHttpResponseCodes::IsOk(L_Code))
		{
			return MakeError(DescribeFailure(L_Code, L_Response->GetContentAsString()));
		}
		return MakeValue(FString());
	}

	struct FUploadFrameOptions
	{
		FString Url;
		FString Authorization;
		FString FileName;
		TArray<uint8> Content;
	};

	TValueOrError<FString, FString> PostFrameSync(const FUploadFrameOptions& InOptions)
	{
		const FHttpRequestPtr L_Request = FHttpModule::Get().CreateRequest();
		L_Request->SetURL(InOptions.Url);
		L_Request->SetVerb(TEXT("POST"));
		L_Request->SetHeader(TEXT("Content-Type"), TEXT("application/octet-stream"));
		L_Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
		L_Request->SetHeader(TEXT("Content-Disposition"), InOptions.FileName);
		if (!InOptions.Authorization.IsEmpty())
		{
			L_Request->SetHeader(TEXT("Authorization"), InOptions.Authorization);
		}
		L_Request->SetContent(InOptions.Content);
		L_Request->SetTimeout(RequestTimeoutSeconds);

		L_Request->ProcessRequestUntilComplete();

		const FHttpResponsePtr L_Response = L_Request->GetResponse();
		if (!L_Response.IsValid())
		{
			return MakeError(FString::Printf(TEXT("无法连接服务器: %s"), *InOptions.Url));
		}
		const int32 L_Code = L_Response->GetResponseCode();
		if (!EHttpResponseCodes::IsOk(L_Code))
		{
			return MakeError(DescribeFailure(L_Code, L_Response->GetContentAsString()));
		}
		return MakeValue(FString());
	}
	FString BuildCandidateUrl(const FString& InServerUrl, const FString& InCandidateId, const TCHAR* InAction)
	{
		return FString::Printf(TEXT("%s/api/doodle/ai/light/candidate/%s/%s"), *InServerUrl, *InCandidateId, InAction);
	}
}

namespace DoodleKitsuClient
{
	FDoodleIterationResult ParseIterationBody(const TSharedPtr<FJsonObject>& InRoot)
	{
		if (!InRoot.IsValid())
		{
			return MakeError(TEXT("轮次响应不是 json 对象"));
		}

		try
		{
			return MakeValue(ParseIterationRoot(InRoot));
		}
		catch (const FString& L_Error)
		{
			return MakeError(L_Error);
		}
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
					UE_LOG(LogDoodleLightingEditor, Error, TEXT("登录失败 %s"), *L_Result.GetError());
					L_OnComplete(MoveTemp(L_Result));
					return;
				}

				const FString L_AccessToken = L_Result.GetValue().AccessToken;
				UE_LOG(LogDoodleLightingEditor, Log, TEXT("登录成功 %s"), *L_Email);

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

	FDoodleUploadSequenceResult UploadCandidateSequence(
		const FUploadSequenceOptions& InOptions
	)
	{
		if (InOptions.CandidateId.IsEmpty())
		{
			return MakeError(FString(TEXT("候选 id 为空")));
		}

		const FString L_ServerUrl = TrimServerUrl(InOptions.ServerUrl);
		if (L_ServerUrl.IsEmpty())
		{
			return MakeError(FString(TEXT("服务器 URL 为空")));
		}
		if (!L_ServerUrl.StartsWith(TEXT("http://")) && !L_ServerUrl.StartsWith(TEXT("https://")))
		{
			return MakeError(FString(TEXT("服务器 URL 必须以 http:// 或 https:// 开头")));
		}

		const FString L_Authorization = ResolveAuthorization();
		if (L_Authorization.IsEmpty())
		{
			return MakeError(FString(TEXT("没有登录令牌，请先登录")));
		}

		const TValueOrError<TArray<FString>, FString> L_Frames = ListPngFrames(InOptions.FramesDir);
		if (L_Frames.HasError())
		{
			return MakeError(L_Frames.GetError());
		}

		const TArray<FString>& L_FramePaths = L_Frames.GetValue();
		const int32 L_FrameRate = InOptions.FrameRate > 0 ? InOptions.FrameRate : 24;

		UE_LOG(
			LogDoodleLightingEditor, Log, TEXT("候选 %s 开始上传帧序列，共 %d 帧，%d fps，目录 %s"), *InOptions.CandidateId,
			L_FramePaths.Num(), L_FrameRate, *InOptions.FramesDir
		);

		FUploadJsonOptions L_BeginOptions;
		L_BeginOptions.Url = BuildCandidateUrl(L_ServerUrl, InOptions.CandidateId, TEXT("begin_sequence"));
		L_BeginOptions.Authorization = L_Authorization;
		L_BeginOptions.Body = TEXT("{}");
		const TValueOrError<FString, FString> L_BeginResult = PostJsonSync(L_BeginOptions);
		if (L_BeginResult.HasError())
		{
			const FString L_Error = FString::Printf(
				TEXT("候选 %s 开始上传失败: %s"), *InOptions.CandidateId, *L_BeginResult.GetError()
			);
			UE_LOG(LogDoodleLightingEditor, Error, TEXT("%s"), *L_Error);
			return MakeError(L_Error);
		}

		const FString L_UploadUrl = BuildCandidateUrl(L_ServerUrl, InOptions.CandidateId, TEXT("upload_frame"));
		for (int32 L_Index = 0; L_Index < L_FramePaths.Num(); ++L_Index)
		{
			const FString& L_Path = L_FramePaths[L_Index];
			TArray<uint8> L_Content;
			if (!FFileHelper::LoadFileToArray(L_Content, *L_Path))
			{
				const FString L_Error = FString::Printf(TEXT("候选 %s 帧文件读取失败: %s"), *InOptions.CandidateId, *L_Path);
				UE_LOG(LogDoodleLightingEditor, Error, TEXT("%s"), *L_Error);
				return MakeError(L_Error);
			}

			const FString L_FileName = FPaths::GetCleanFilename(L_Path);
			UE_LOG(
				LogDoodleLightingEditor, Log, TEXT("候选 %s 上传帧 %d/%d：%s"), *InOptions.CandidateId, L_Index + 1,
				L_FramePaths.Num(), *L_FileName
			);

			FUploadFrameOptions L_FrameOptions;
			L_FrameOptions.Url = L_UploadUrl;
			L_FrameOptions.Authorization = L_Authorization;
			L_FrameOptions.FileName = L_FileName;
			L_FrameOptions.Content = MoveTemp(L_Content);
			const TValueOrError<FString, FString> L_FrameResult = PostFrameSync(L_FrameOptions);
			if (L_FrameResult.HasError())
			{
				const FString L_Error = FString::Printf(
					TEXT("候选 %s 上传第 %d 帧失败: %s"), *InOptions.CandidateId, L_Index + 1, *L_FrameResult.GetError()
				);
				UE_LOG(LogDoodleLightingEditor, Error, TEXT("%s"), *L_Error);
				return MakeError(L_Error);
			}
		}

		FString L_FinishBody = TEXT("{}");
		TSharedRef<FJsonObject> L_RequestRoot = MakeShared<FJsonObject>();
		L_RequestRoot->SetNumberField(TEXT("frame_rate"), L_FrameRate);
		FString L_Serialized;
		const TSharedRef<TJsonWriter<>> L_Writer = TJsonWriterFactory<>::Create(&L_Serialized);
		if (FJsonSerializer::Serialize(L_RequestRoot, L_Writer))
		{
			L_FinishBody = L_Serialized;
		}

		FUploadJsonOptions L_FinishOptions;
		L_FinishOptions.Url = BuildCandidateUrl(L_ServerUrl, InOptions.CandidateId, TEXT("finish_sequence"));
		L_FinishOptions.Authorization = L_Authorization;
		L_FinishOptions.Body = L_FinishBody;
		const TValueOrError<FString, FString> L_FinishResult = PostJsonSync(L_FinishOptions);
		if (L_FinishResult.HasError())
		{
			const FString L_Error = FString::Printf(
				TEXT("候选 %s 收尾失败: %s"), *InOptions.CandidateId, *L_FinishResult.GetError()
			);
			UE_LOG(LogDoodleLightingEditor, Error, TEXT("%s"), *L_Error);
			return MakeError(L_Error);
		}

		UE_LOG(
			LogDoodleLightingEditor, Log, TEXT("候选 %s 帧序列上传完成，共 %d 帧"), *InOptions.CandidateId, L_FramePaths.Num()
		);

		FDoodleUploadSequenceValue L_Value;
		L_Value.CandidateId = InOptions.CandidateId;
		L_Value.FrameCount = L_FramePaths.Num();
		return MakeValue(MoveTemp(L_Value));
	}
}
