#include "DoodleLightingJson.h"

namespace
{
	bool Fail(FString& OutError, const FString& InPath, const FString& InReason)
	{
		OutError = FString::Printf(TEXT("%s：%s"), *InPath, *InReason);
		return false;
	}

	bool GetObject(const TSharedPtr<FJsonObject>& In, const TCHAR* InKey, const FString& InPath, TSharedPtr<FJsonObject>& OutValue, FString& OutError)
	{
		const TSharedPtr<FJsonObject>* LObject = nullptr;
		if (!In->TryGetObjectField(InKey, LObject) || LObject == nullptr || !LObject->IsValid())
		{
			return Fail(OutError, InPath, TEXT("缺失或不是对象"));
		}
		OutValue = *LObject;
		return true;
	}

	bool TryGetObjectFieldPtr(const TSharedPtr<FJsonObject>& In, const TCHAR* InKey, TSharedPtr<FJsonObject>& OutValue)
	{
		const TSharedPtr<FJsonObject>* LObject = nullptr;
		if (!In->TryGetObjectField(InKey, LObject) || LObject == nullptr) return false;
		OutValue = *LObject;
		return true;
	}

	bool GetArray(const TSharedPtr<FJsonObject>& In, const TCHAR* InKey, const FString& InPath, const TArray<TSharedPtr<FJsonValue>>*& OutValue, FString& OutError)
	{
		if (!In->TryGetArrayField(InKey, OutValue) || OutValue == nullptr)
		{
			return Fail(OutError, InPath, TEXT("缺失或不是数组"));
		}
		return true;
	}

	bool GetString(const TSharedPtr<FJsonObject>& In, const TCHAR* InKey, const FString& InPath, FString& OutValue, FString& OutError, bool bRequired = true)
	{
		if (!In->TryGetStringField(InKey, OutValue))
		{
			if (!bRequired)
			{
				return true;
			}
			return Fail(OutError, InPath, TEXT("缺失或不是字符串"));
		}
		return true;
	}

	bool GetNumber(const TSharedPtr<FJsonObject>& In, const TCHAR* InKey, const FString& InPath, double& OutValue, FString& OutError, bool bRequired = true)
	{
		if (!In->TryGetNumberField(InKey, OutValue))
		{
			if (!bRequired)
			{
				return true;
			}
			return Fail(OutError, InPath, TEXT("缺失或不是数字"));
		}
		return true;
	}

	bool GetInt(const TSharedPtr<FJsonObject>& In, const TCHAR* InKey, const FString& InPath, int32& OutValue, FString& OutError, bool bRequired = true)
	{
		if (!In->TryGetNumberField(InKey, OutValue))
		{
			if (!bRequired)
			{
				return true;
			}
			return Fail(OutError, InPath, TEXT("缺失或不是整数"));
		}
		return true;
	}

	bool GetInt64(const TSharedPtr<FJsonObject>& In, const TCHAR* InKey, const FString& InPath, int64& OutValue, FString& OutError, bool bRequired = true)
	{
		if (!In->TryGetNumberField(InKey, OutValue))
		{
			if (!bRequired)
			{
				return true;
			}
			return Fail(OutError, InPath, TEXT("缺失或不是整数"));
		}
		return true;
	}

	bool GetBool(const TSharedPtr<FJsonObject>& In, const TCHAR* InKey, const FString& InPath, bool& OutValue, FString& OutError, bool bRequired = true)
	{
		if (!In->TryGetBoolField(InKey, OutValue))
		{
			if (!bRequired)
			{
				return true;
			}
			return Fail(OutError, InPath, TEXT("缺失或不是布尔"));
		}
		return true;
	}

	bool GetVec3(const TSharedPtr<FJsonObject>& In, const TCHAR* InKey, const FString& InPath, FDoodleVec3& OutValue, FString& OutError)
	{
		const TArray<TSharedPtr<FJsonValue>>* LArray = nullptr;
		if (!GetArray(In, InKey, InPath, LArray, OutError))
		{
			return false;
		}
		if (LArray->Num() != 3)
		{
			return Fail(OutError, InPath, FString::Printf(TEXT("需要长度为 3 的数组，实际 %d"), LArray->Num()));
		}
		double LValues[3] = {0.0, 0.0, 0.0};
		for (int32 LIndex = 0; LIndex < 3; ++LIndex)
		{
			if (!(*LArray)[LIndex].IsValid() || !(*LArray)[LIndex]->TryGetNumber(LValues[LIndex]))
			{
				return Fail(OutError, FString::Printf(TEXT("%s[%d]"), *InPath, LIndex), TEXT("不是数字"));
			}
		}
		OutValue = FDoodleVec3(LValues[0], LValues[1], LValues[2]);
		return true;
	}

	template <typename EnumType, typename ConvertType>
	bool GetEnum(const TSharedPtr<FJsonObject>& In, const TCHAR* InKey, const FString& InPath, EnumType& OutValue, FString& OutError, ConvertType InConvert, bool bRequired = true)
	{
		FString LText;
		if (!GetString(In, InKey, InPath, LText, OutError, bRequired))
		{
			return false;
		}
		if (LText.IsEmpty() && !bRequired)
		{
			return true;
		}
		if (!InConvert(LText, OutValue))
		{
			return Fail(OutError, InPath, FString::Printf(TEXT("值 \"%s\" 不是合法枚举"), *LText));
		}
		return true;
	}

	bool ParseAnchor(const TSharedPtr<FJsonObject>& In, const FString& InPath, FDoodleAnchor& OutValue, FString& OutError)
	{
		TSharedPtr<FJsonObject> LObject;
		if (!GetObject(In, TEXT("anchor"), InPath, LObject, OutError))
		{
			return false;
		}
		if (!GetEnum(LObject, TEXT("space"), InPath + TEXT(".space"), OutValue.Space, OutError, &DoodleAnchorSpaceFromString))
		{
			return false;
		}
		GetString(LObject, TEXT("target"), InPath + TEXT(".target"), OutValue.Target, OutError, false);
		GetString(LObject, TEXT("look_at"), InPath + TEXT(".look_at"), OutValue.LookAt, OutError, false);
		return true;
	}

	bool ParseOffset(const TSharedPtr<FJsonObject>& In, const FString& InPath, FDoodleOffset& OutValue, FString& OutError)
	{
		TSharedPtr<FJsonObject> LObject;
		if (!GetObject(In, TEXT("offset"), InPath, LObject, OutError))
		{
			return false;
		}
		if (!GetNumber(LObject, TEXT("azimuth_deg"), InPath + TEXT(".azimuth_deg"), OutValue.AzimuthDeg, OutError)) return false;
		if (!GetNumber(LObject, TEXT("elevation_deg"), InPath + TEXT(".elevation_deg"), OutValue.ElevationDeg, OutError)) return false;
		if (!GetNumber(LObject, TEXT("distance_m"), InPath + TEXT(".distance_m"), OutValue.DistanceM, OutError)) return false;
		return true;
	}

	bool ParseColor(const TSharedPtr<FJsonObject>& In, const FString& InPath, FDoodleColor& OutValue, FString& OutError)
	{
		TSharedPtr<FJsonObject> LObject;
		if (!GetObject(In, TEXT("color"), InPath, LObject, OutError))
		{
			return false;
		}
		if (!GetEnum(LObject, TEXT("mode"), InPath + TEXT(".mode"), OutValue.Mode, OutError, &DoodleColorModeFromString))
		{
			return false;
		}
		switch (OutValue.Mode)
		{
			case EDoodleColorMode::Temperature:
				return GetNumber(LObject, TEXT("kelvin"), InPath + TEXT(".kelvin"), OutValue.Kelvin, OutError);
			case EDoodleColorMode::RGB:
				return GetVec3(LObject, TEXT("rgb"), InPath + TEXT(".rgb"), OutValue.RGB, OutError);
			case EDoodleColorMode::Filter:
				return GetString(LObject, TEXT("filter"), InPath + TEXT(".filter"), OutValue.Filter, OutError);
			default:
				return true;
		}
	}

	bool ParseShape(const TSharedPtr<FJsonObject>& In, const FString& InPath, FDoodleShape& OutValue, FString& OutError)
	{
		TSharedPtr<FJsonObject> LObject;
		if (!GetObject(In, TEXT("shape"), InPath, LObject, OutError))
		{
			return false;
		}
		if (!GetNumber(LObject, TEXT("source_radius_cm"), InPath + TEXT(".source_radius_cm"), OutValue.SourceRadiusCm, OutError)) return false;
		if (!GetString(LObject, TEXT("ies"), InPath + TEXT(".ies"), OutValue.Ies, OutError)) return false;
		double LCone = 0.0;
		if (LObject->TryGetNumberField(TEXT("inner_cone_deg"), LCone)) OutValue.InnerConeDeg = LCone;
		if (LObject->TryGetNumberField(TEXT("outer_cone_deg"), LCone)) OutValue.OuterConeDeg = LCone;
		return true;
	}

	bool ParseShadows(const TSharedPtr<FJsonObject>& In, const FString& InPath, FDoodleShadows& OutValue, FString& OutError)
	{
		TSharedPtr<FJsonObject> LObject;
		if (!GetObject(In, TEXT("shadows"), InPath, LObject, OutError))
		{
			return false;
		}
		if (!GetBool(LObject, TEXT("cast"), InPath + TEXT(".cast"), OutValue.bCast, OutError)) return false;
		if (!GetNumber(LObject, TEXT("softness"), InPath + TEXT(".softness"), OutValue.Softness, OutError)) return false;
		return true;
	}

	bool ParseIntensity(const TSharedPtr<FJsonObject>& In, const FString& InPath, FDoodleIntensity& OutValue, FString& OutError)
	{
		TSharedPtr<FJsonObject> LObject;
		if (!GetObject(In, TEXT("intensity"), InPath, LObject, OutError))
		{
			return false;
		}
		if (!GetEnum(LObject, TEXT("unit"), InPath + TEXT(".unit"), OutValue.Unit, OutError, &DoodleIntensityUnitFromString)) return false;
		if (!GetNumber(LObject, TEXT("value"), InPath + TEXT(".value"), OutValue.Value, OutError)) return false;
		return true;
	}

	bool ParseLight(const TSharedPtr<FJsonObject>& In, const FString& InPath, FDoodleLight& OutValue, FString& OutError)
	{
		if (!GetString(In, TEXT("id"), InPath + TEXT(".id"), OutValue.Id, OutError)) return false;
		if (!GetEnum(In, TEXT("role"), InPath + TEXT(".role"), OutValue.Role, OutError, &DoodleLightRoleFromString)) return false;
		if (!GetEnum(In, TEXT("type"), InPath + TEXT(".type"), OutValue.Type, OutError, &DoodleLightTypeFromString)) return false;
		if (!ParseAnchor(In, InPath + TEXT(".anchor"), OutValue.Anchor, OutError)) return false;
		if (!ParseOffset(In, InPath + TEXT(".offset"), OutValue.Offset, OutError)) return false;
		if (!ParseIntensity(In, InPath + TEXT(".intensity"), OutValue.Intensity, OutError)) return false;
		if (!ParseColor(In, InPath + TEXT(".color"), OutValue.Color, OutError)) return false;
		if (!ParseShape(In, InPath + TEXT(".shape"), OutValue.Shape, OutError)) return false;
		if (!ParseShadows(In, InPath + TEXT(".shadows"), OutValue.Shadows, OutError)) return false;
		if (!GetEnum(In, TEXT("mobility"), InPath + TEXT(".mobility"), OutValue.Mobility, OutError, &DoodleLightMobilityFromString)) return false;
		if (!GetEnum(In, TEXT("origin"), InPath + TEXT(".origin"), OutValue.Origin, OutError, &DoodleLightOriginFromString)) return false;

		const TArray<TSharedPtr<FJsonValue>>* LChannels = nullptr;
		if (!GetArray(In, TEXT("channels"), InPath + TEXT(".channels"), LChannels, OutError)) return false;
		OutValue.Channels.Reset();
		for (int32 LIndex = 0; LIndex < LChannels->Num(); ++LIndex)
		{
			int32 LValue = 0;
			if (!(*LChannels)[LIndex].IsValid() || !(*LChannels)[LIndex]->TryGetNumber(LValue))
			{
				return Fail(OutError, FString::Printf(TEXT("%s.channels[%d]"), *InPath, LIndex), TEXT("不是整数"));
			}
			OutValue.Channels.Add(LValue);
		}

		const TArray<TSharedPtr<FJsonValue>>* LAnim = nullptr;
		if (!GetArray(In, TEXT("anim"), InPath + TEXT(".anim"), LAnim, OutError)) return false;
		OutValue.Anim.Reset();
		for (int32 LIndex = 0; LIndex < LAnim->Num(); ++LIndex)
		{
			const TSharedPtr<FJsonObject>* LKey = nullptr;
			if (!(*LAnim)[LIndex].IsValid() || !(*LAnim)[LIndex]->TryGetObject(LKey) || LKey == nullptr)
			{
				return Fail(OutError, FString::Printf(TEXT("%s.anim[%d]"), *InPath, LIndex), TEXT("不是对象"));
			}
			FDoodleAnimKey LAnimKey;
			const FString LKeyPath = FString::Printf(TEXT("%s.anim[%d]"), *InPath, LIndex);
			if (!GetInt(*LKey, TEXT("frame"), LKeyPath + TEXT(".frame"), LAnimKey.Frame, OutError)) return false;
			if (!GetNumber(*LKey, TEXT("intensity"), LKeyPath + TEXT(".intensity"), LAnimKey.Intensity, OutError)) return false;
			OutValue.Anim.Add(LAnimKey);
		}
		return true;
	}

	bool ParseGlobals(const TSharedPtr<FJsonObject>& In, const FString& InPath, FDoodleGlobals& OutValue, FString& OutError)
	{
		TSharedPtr<FJsonObject> LObject;
		if (!GetObject(In, TEXT("globals"), InPath, LObject, OutError))
		{
			return false;
		}

		TSharedPtr<FJsonObject> LSub;
		if (!GetObject(LObject, TEXT("exposure"), InPath + TEXT(".exposure"), LSub, OutError)) return false;
		if (!GetString(LSub, TEXT("method"), InPath + TEXT(".exposure.method"), OutValue.Exposure.Method, OutError)) return false;
		if (!GetNumber(LSub, TEXT("ev100_bias"), InPath + TEXT(".exposure.ev100_bias"), OutValue.Exposure.Ev100Bias, OutError)) return false;

		if (!GetObject(LObject, TEXT("white_balance"), InPath + TEXT(".white_balance"), LSub, OutError)) return false;
		if (!GetNumber(LSub, TEXT("kelvin"), InPath + TEXT(".white_balance.kelvin"), OutValue.WhiteBalance.Kelvin, OutError)) return false;
		if (!GetNumber(LSub, TEXT("tint"), InPath + TEXT(".white_balance.tint"), OutValue.WhiteBalance.Tint, OutError)) return false;

		if (!GetObject(LObject, TEXT("environment"), InPath + TEXT(".environment"), LSub, OutError)) return false;
		if (!GetNumber(LSub, TEXT("sky_light_intensity"), InPath + TEXT(".environment.sky_light_intensity"), OutValue.Environment.SkyLightIntensity, OutError)) return false;
		if (!GetNumber(LSub, TEXT("fog_density"), InPath + TEXT(".environment.fog_density"), OutValue.Environment.FogDensity, OutError)) return false;

		if (!GetObject(LObject, TEXT("grading"), InPath + TEXT(".grading"), LSub, OutError)) return false;
		if (!GetNumber(LSub, TEXT("contrast"), InPath + TEXT(".grading.contrast"), OutValue.Grading.Contrast, OutError)) return false;
		if (!GetNumber(LSub, TEXT("saturation"), InPath + TEXT(".grading.saturation"), OutValue.Grading.Saturation, OutError)) return false;
		if (!GetNumber(LSub, TEXT("lift"), InPath + TEXT(".grading.lift"), OutValue.Grading.Lift, OutError)) return false;
		if (!GetNumber(LSub, TEXT("vignette"), InPath + TEXT(".grading.vignette"), OutValue.Grading.Vignette, OutError)) return false;
		if (!GetNumber(LSub, TEXT("bloom"), InPath + TEXT(".grading.bloom"), OutValue.Grading.Bloom, OutError)) return false;
		return true;
	}

	void WriteVec3(const TSharedRef<FJsonObject>& In, const TCHAR* InKey, const FDoodleVec3& InValue)
	{
		TArray<TSharedPtr<FJsonValue>> LArray;
		LArray.Add(MakeShared<FJsonValueNumber>(InValue.X));
		LArray.Add(MakeShared<FJsonValueNumber>(InValue.Y));
		LArray.Add(MakeShared<FJsonValueNumber>(InValue.Z));
		In->SetArrayField(InKey, LArray);
	}
}

bool DoodleLightingJson::ParseLightingRig(const TSharedPtr<FJsonObject>& InRoot, FDoodleLightingRig& OutRig, FString& OutError)
{
	if (!InRoot.IsValid())
	{
		OutError = TEXT("rig：不是 JSON 对象");
		return false;
	}

	GetString(InRoot, TEXT("schema_version"), TEXT("rig.schema_version"), OutRig.SchemaVersion, OutError, false);
	GetString(InRoot, TEXT("rig_id"), TEXT("rig.rig_id"), OutRig.RigId, OutError, false);
	GetString(InRoot, TEXT("session_id"), TEXT("rig.session_id"), OutRig.SessionId, OutError, false);
	GetString(InRoot, TEXT("shot_task_id"), TEXT("rig.shot_task_id"), OutRig.ShotTaskId, OutError, false);
	GetString(InRoot, TEXT("spec_id"), TEXT("rig.spec_id"), OutRig.SpecId, OutError, false);
	GetString(InRoot, TEXT("baseline_level"), TEXT("rig.baseline_level"), OutRig.BaselineLevel, OutError, false);
	GetInt(InRoot, TEXT("iteration"), TEXT("rig.iteration"), OutRig.Iteration, OutError, false);
	GetInt(InRoot, TEXT("candidate_index"), TEXT("rig.candidate_index"), OutRig.CandidateIndex, OutError, false);

	if (!ParseGlobals(InRoot, TEXT("rig.globals"), OutRig.Globals, OutError))
	{
		return false;
	}

	TSharedPtr<FJsonObject> LObject;
	if (TryGetObjectFieldPtr(InRoot, TEXT("provenance"), LObject) && LObject.IsValid())
	{
		GetString(LObject, TEXT("model"), TEXT("rig.provenance.model"), OutRig.Provenance.Model, OutError, false);
		GetString(LObject, TEXT("prompt_hash"), TEXT("rig.provenance.prompt_hash"), OutRig.Provenance.PromptHash, OutError, false);
		GetString(LObject, TEXT("reference"), TEXT("rig.provenance.reference"), OutRig.Provenance.Reference, OutError, false);
		GetInt(LObject, TEXT("seed"), TEXT("rig.provenance.seed"), OutRig.Provenance.Seed, OutError, false);
		GetString(LObject, TEXT("tier"), TEXT("rig.provenance.tier"), OutRig.Provenance.Tier, OutError, false);
	}

	const TArray<TSharedPtr<FJsonValue>>* LLights = nullptr;
	if (!GetArray(InRoot, TEXT("lights"), TEXT("rig.lights"), LLights, OutError))
	{
		return false;
	}
	if (LLights->Num() == 0)
	{
		OutError = TEXT("rig.lights：数组为空（至少要有一盏灯）");
		return false;
	}
	OutRig.Lights.Reset();
	for (int32 LIndex = 0; LIndex < LLights->Num(); ++LIndex)
	{
		const TSharedPtr<FJsonObject>* LItem = nullptr;
		const FString LPath = FString::Printf(TEXT("rig.lights[%d]"), LIndex);
		if (!(*LLights)[LIndex].IsValid() || !(*LLights)[LIndex]->TryGetObject(LItem) || LItem == nullptr)
		{
			return Fail(OutError, LPath, TEXT("不是对象"));
		}
		FDoodleLight LLight;
		if (!ParseLight(*LItem, LPath, LLight, OutError))
		{
			return false;
		}
		OutRig.Lights.Add(MoveTemp(LLight));
	}

	const TArray<TSharedPtr<FJsonValue>>* LOps = nullptr;
	if (InRoot->TryGetArrayField(TEXT("ops"), LOps) && LOps != nullptr)
	{
		OutRig.Ops.Reset();
		for (int32 LIndex = 0; LIndex < LOps->Num(); ++LIndex)
		{
			const TSharedPtr<FJsonObject>* LItem = nullptr;
			const FString LPath = FString::Printf(TEXT("rig.ops[%d]"), LIndex);
			if (!(*LOps)[LIndex].IsValid() || !(*LOps)[LIndex]->TryGetObject(LItem) || LItem == nullptr)
			{
				return Fail(OutError, LPath, TEXT("不是对象"));
			}
			FDoodleLightOp LOp;
			if (!GetEnum(*LItem, TEXT("op"), LPath + TEXT(".op"), LOp.Op, OutError, &DoodleLightOpKindFromString))
			{
				return false;
			}
			GetString(*LItem, TEXT("light"), LPath + TEXT(".light"), LOp.Light, OutError, false);
			GetString(*LItem, TEXT("rationale"), LPath + TEXT(".rationale"), LOp.Rationale, OutError, false);
			TSharedPtr<FJsonObject> LPatch;
			if (TryGetObjectFieldPtr(*LItem, TEXT("patch"), LPatch) && LPatch.IsValid())
			{
				LOp.Patch = LPatch;
			}
			OutRig.Ops.Add(MoveTemp(LOp));
		}
	}
	return true;
}

bool DoodleLightingJson::ParseTaskPayload(const TSharedPtr<FJsonObject>& InRoot, FDoodleTaskPayload& OutPayload, FString& OutError)
{
	if (!InRoot.IsValid())
	{
		OutError = TEXT("payload：不是 JSON 对象");
		return false;
	}

	if (!GetString(InRoot, TEXT("session_id"), TEXT("session_id"), OutPayload.SessionId, OutError)) return false;
	if (!GetInt(InRoot, TEXT("iteration"), TEXT("iteration"), OutPayload.Iteration, OutError)) return false;
	GetString(InRoot, TEXT("spec_id"), TEXT("spec_id"), OutPayload.SpecId, OutError, false);
	if (!GetString(InRoot, TEXT("out_dir"), TEXT("out_dir"), OutPayload.OutDir, OutError)) return false;
	GetString(InRoot, TEXT("baseline_level"), TEXT("baseline_level"), OutPayload.BaselineLevel, OutError, false);
	GetString(InRoot, TEXT("ue_project_path"), TEXT("ue_project_path"), OutPayload.UeProjectPath, OutError, false);
	GetString(InRoot, TEXT("camera_file_path"), TEXT("camera_file_path"), OutPayload.CameraFilePath, OutError, false);
	if (!GetString(InRoot, TEXT("render_map"), TEXT("render_map"), OutPayload.RenderMap, OutError)) return false;
	if (!GetString(InRoot, TEXT("level_sequence"), TEXT("level_sequence"), OutPayload.LevelSequence, OutError)) return false;
	GetString(InRoot, TEXT("movie_pipeline_config"), TEXT("movie_pipeline_config"), OutPayload.MoviePipelineConfig, OutError, false);
	GetInt(InRoot, TEXT("frame_begin"), TEXT("frame_begin"), OutPayload.FrameBegin, OutError, false);
	GetInt(InRoot, TEXT("frame_end"), TEXT("frame_end"), OutPayload.FrameEnd, OutError, false);

	const TArray<TSharedPtr<FJsonValue>>* LCandidates = nullptr;
	if (!GetArray(InRoot, TEXT("candidates"), TEXT("candidates"), LCandidates, OutError)) return false;
	if (LCandidates->Num() == 0)
	{
		OutError = TEXT("candidates：数组为空");
		return false;
	}

	OutPayload.Candidates.Reset();
	for (int32 LIndex = 0; LIndex < LCandidates->Num(); ++LIndex)
	{
		const TSharedPtr<FJsonObject>* LItem = nullptr;
		const FString LPath = FString::Printf(TEXT("candidates[%d]"), LIndex);
		if (!(*LCandidates)[LIndex].IsValid() || !(*LCandidates)[LIndex]->TryGetObject(LItem) || LItem == nullptr)
		{
			return Fail(OutError, LPath, TEXT("不是对象"));
		}
		FDoodleTaskCandidate LCandidate;
		if (!GetInt(*LItem, TEXT("candidate_index"), LPath + TEXT(".candidate_index"), LCandidate.CandidateIndex, OutError)) return false;
		if (!GetString(*LItem, TEXT("candidate_id"), LPath + TEXT(".candidate_id"), LCandidate.CandidateId, OutError)) return false;
		if (!GetString(*LItem, TEXT("frames_dir"), LPath + TEXT(".frames_dir"), LCandidate.FramesDir, OutError)) return false;
		GetString(*LItem, TEXT("png_path"), LPath + TEXT(".png_path"), LCandidate.PngPath, OutError, false);
		GetString(*LItem, TEXT("exr_path"), LPath + TEXT(".exr_path"), LCandidate.ExrPath, OutError, false);
		GetString(*LItem, TEXT("movie_path"), LPath + TEXT(".movie_path"), LCandidate.MoviePath, OutError, false);
		TSharedPtr<FJsonObject> LRig;
		if (!GetObject(*LItem, TEXT("rig"), LPath + TEXT(".rig"), LRig, OutError)) return false;
		if (!ParseLightingRig(LRig, LCandidate.Rig, OutError)) return false;
		OutPayload.Candidates.Add(MoveTemp(LCandidate));
	}
	return true;
}

TSharedPtr<FJsonObject> DoodleLightingJson::SerializeTaskPayload(const FDoodleTaskPayload& InPayload)
{
	TSharedRef<FJsonObject> LRoot = MakeShared<FJsonObject>();
	LRoot->SetStringField(TEXT("session_id"), InPayload.SessionId);
	LRoot->SetNumberField(TEXT("iteration"), InPayload.Iteration);
	LRoot->SetStringField(TEXT("spec_id"), InPayload.SpecId);
	LRoot->SetStringField(TEXT("out_dir"), InPayload.OutDir);
	LRoot->SetStringField(TEXT("baseline_level"), InPayload.BaselineLevel);
	LRoot->SetStringField(TEXT("ue_project_path"), InPayload.UeProjectPath);
	LRoot->SetStringField(TEXT("camera_file_path"), InPayload.CameraFilePath);
	LRoot->SetStringField(TEXT("render_map"), InPayload.RenderMap);
	LRoot->SetStringField(TEXT("level_sequence"), InPayload.LevelSequence);
	LRoot->SetStringField(TEXT("movie_pipeline_config"), InPayload.MoviePipelineConfig);
	LRoot->SetNumberField(TEXT("frame_begin"), InPayload.FrameBegin);
	LRoot->SetNumberField(TEXT("frame_end"), InPayload.FrameEnd);

	TArray<TSharedPtr<FJsonValue>> LCandidates;
	for (const FDoodleTaskCandidate& LCandidate : InPayload.Candidates)
	{
		TSharedRef<FJsonObject> LItem = MakeShared<FJsonObject>();
		LItem->SetNumberField(TEXT("candidate_index"), LCandidate.CandidateIndex);
		LItem->SetStringField(TEXT("candidate_id"), LCandidate.CandidateId);
		LItem->SetStringField(TEXT("frames_dir"), LCandidate.FramesDir);
		LItem->SetStringField(TEXT("png_path"), LCandidate.PngPath);
		LItem->SetStringField(TEXT("exr_path"), LCandidate.ExrPath);
		LItem->SetStringField(TEXT("movie_path"), LCandidate.MoviePath);
		LCandidates.Add(MakeShared<FJsonValueObject>(LItem));
	}
	LRoot->SetArrayField(TEXT("candidates"), LCandidates);
	return LRoot;
}

TSharedPtr<FJsonObject> DoodleLightingJson::SerializeLaunchPlan(const FDoodleLaunchPlan& InPlan)
{
	TSharedRef<FJsonObject> LRoot = MakeShared<FJsonObject>();
	LRoot->SetStringField(TEXT("session_id"), InPlan.SessionId);
	LRoot->SetStringField(TEXT("iteration_id"), InPlan.IterationId);
	LRoot->SetNumberField(TEXT("iteration"), InPlan.Iteration);
	LRoot->SetStringField(TEXT("out_dir"), InPlan.OutDir);
	LRoot->SetStringField(TEXT("render_map"), InPlan.RenderMap);
	LRoot->SetStringField(TEXT("level_sequence"), InPlan.LevelSequence);
	LRoot->SetStringField(TEXT("movie_pipeline_config"), InPlan.MoviePipelineConfig);
	LRoot->SetNumberField(TEXT("frame_begin"), InPlan.FrameBegin);
	LRoot->SetNumberField(TEXT("frame_end"), InPlan.FrameEnd);
	LRoot->SetStringField(TEXT("ue_project_path"), InPlan.UeProjectPath);
	LRoot->SetStringField(TEXT("camera_file_path"), InPlan.CameraFilePath);

	TArray<TSharedPtr<FJsonValue>> LCandidates;
	for (const FDoodleTaskCandidate& LCandidate : InPlan.Candidates)
	{
		TSharedRef<FJsonObject> LItem = MakeShared<FJsonObject>();
		LItem->SetNumberField(TEXT("candidate_index"), LCandidate.CandidateIndex);
		LItem->SetStringField(TEXT("candidate_id"), LCandidate.CandidateId);
		LItem->SetStringField(TEXT("frames_dir"), LCandidate.FramesDir);
		LItem->SetStringField(TEXT("png_path"), LCandidate.PngPath);
		LItem->SetStringField(TEXT("exr_path"), LCandidate.ExrPath);
		LItem->SetStringField(TEXT("movie_path"), LCandidate.MoviePath);
		LCandidates.Add(MakeShared<FJsonValueObject>(LItem));
	}
	LRoot->SetArrayField(TEXT("candidates"), LCandidates);
	return LRoot;
}

bool DoodleLightingJson::ParseLaunchPlan(const TSharedPtr<FJsonObject>& InRoot, FDoodleLaunchPlan& OutPlan, FString& OutError)
{
	if (!InRoot.IsValid())
	{
		OutError = TEXT("launch_plan：不是 JSON 对象");
		return false;
	}

	if (!GetString(InRoot, TEXT("session_id"), TEXT("session_id"), OutPlan.SessionId, OutError)) return false;
	if (!GetString(InRoot, TEXT("iteration_id"), TEXT("iteration_id"), OutPlan.IterationId, OutError)) return false;
	if (!GetInt(InRoot, TEXT("iteration"), TEXT("iteration"), OutPlan.Iteration, OutError)) return false;
	if (!GetString(InRoot, TEXT("out_dir"), TEXT("out_dir"), OutPlan.OutDir, OutError)) return false;
	if (!GetString(InRoot, TEXT("render_map"), TEXT("render_map"), OutPlan.RenderMap, OutError)) return false;
	if (!GetString(InRoot, TEXT("level_sequence"), TEXT("level_sequence"), OutPlan.LevelSequence, OutError)) return false;
	GetString(InRoot, TEXT("movie_pipeline_config"), TEXT("movie_pipeline_config"), OutPlan.MoviePipelineConfig, OutError, false);
	GetInt(InRoot, TEXT("frame_begin"), TEXT("frame_begin"), OutPlan.FrameBegin, OutError, false);
	GetInt(InRoot, TEXT("frame_end"), TEXT("frame_end"), OutPlan.FrameEnd, OutError, false);
	GetString(InRoot, TEXT("ue_project_path"), TEXT("ue_project_path"), OutPlan.UeProjectPath, OutError, false);
	GetString(InRoot, TEXT("camera_file_path"), TEXT("camera_file_path"), OutPlan.CameraFilePath, OutError, false);

	const TArray<TSharedPtr<FJsonValue>>* LCandidates = nullptr;
	if (!GetArray(InRoot, TEXT("candidates"), TEXT("candidates"), LCandidates, OutError)) return false;
	if (LCandidates->Num() == 0)
	{
		OutError = TEXT("candidates：数组为空");
		return false;
	}

	OutPlan.Candidates.Reset();
	for (int32 LIndex = 0; LIndex < LCandidates->Num(); ++LIndex)
	{
		const TSharedPtr<FJsonObject>* LItem = nullptr;
		const FString LPath = FString::Printf(TEXT("candidates[%d]"), LIndex);
		if (!(*LCandidates)[LIndex].IsValid() || !(*LCandidates)[LIndex]->TryGetObject(LItem) || LItem == nullptr)
		{
			return Fail(OutError, LPath, TEXT("不是对象"));
		}
		FDoodleTaskCandidate LCandidate;
		if (!GetInt(*LItem, TEXT("candidate_index"), LPath + TEXT(".candidate_index"), LCandidate.CandidateIndex, OutError)) return false;
		if (!GetString(*LItem, TEXT("candidate_id"), LPath + TEXT(".candidate_id"), LCandidate.CandidateId, OutError)) return false;
		if (!GetString(*LItem, TEXT("frames_dir"), LPath + TEXT(".frames_dir"), LCandidate.FramesDir, OutError)) return false;
		GetString(*LItem, TEXT("png_path"), LPath + TEXT(".png_path"), LCandidate.PngPath, OutError, false);
		GetString(*LItem, TEXT("exr_path"), LPath + TEXT(".exr_path"), LCandidate.ExrPath, OutError, false);
		GetString(*LItem, TEXT("movie_path"), LPath + TEXT(".movie_path"), LCandidate.MoviePath, OutError, false);
		OutPlan.Candidates.Add(MoveTemp(LCandidate));
	}
	return true;
}

TSharedPtr<FJsonObject> DoodleLightingJson::SerializeRoundReport(const FDoodleRoundReport& InReport)
{
	TSharedRef<FJsonObject> LRoot = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> LCandidates;
	for (const FDoodleCandidateReport& LCandidate : InReport.Candidates)
	{
		TSharedRef<FJsonObject> LItem = MakeShared<FJsonObject>();
		LItem->SetNumberField(TEXT("candidate_index"), LCandidate.CandidateIndex);
		LItem->SetStringField(TEXT("candidate_id"), LCandidate.CandidateId);
		LItem->SetStringField(TEXT("status"), DoodleRenderStatusToString(LCandidate.Status));
		LItem->SetNumberField(TEXT("render_ms"), LCandidate.RenderMs);
		LItem->SetNumberField(TEXT("light_count"), LCandidate.LightCount);
		LItem->SetNumberField(TEXT("frame_count"), LCandidate.FrameCount);
		if (LCandidate.Metrics.IsValid())
		{
			LItem->SetObjectField(TEXT("metrics"), LCandidate.Metrics);
		}
		LCandidates.Add(MakeShared<FJsonValueObject>(LItem));
	}
	LRoot->SetArrayField(TEXT("candidates"), LCandidates);
	LRoot->SetNumberField(TEXT("frame_rate"), InReport.FrameRate);
	LRoot->SetStringField(TEXT("error"), InReport.Error);
	return LRoot;
}

bool DoodleLightingJson::ParseRoundReport(const TSharedPtr<FJsonObject>& InRoot, FDoodleRoundReport& OutReport, FString& OutError)
{
	if (!InRoot.IsValid())
	{
		OutError = TEXT("render_report：不是 JSON 对象");
		return false;
	}
	const TArray<TSharedPtr<FJsonValue>>* LCandidates = nullptr;
	if (!GetArray(InRoot, TEXT("candidates"), TEXT("candidates"), LCandidates, OutError))
	{
		return false;
	}
	GetString(InRoot, TEXT("error"), TEXT("error"), OutReport.Error, OutError, false);
	GetInt(InRoot, TEXT("frame_rate"), TEXT("frame_rate"), OutReport.FrameRate, OutError, false);

	OutReport.Candidates.Reset();
	for (int32 LIndex = 0; LIndex < LCandidates->Num(); ++LIndex)
	{
		const TSharedPtr<FJsonObject>* LItem = nullptr;
		const FString LPath = FString::Printf(TEXT("candidates[%d]"), LIndex);
		if (!(*LCandidates)[LIndex].IsValid() || !(*LCandidates)[LIndex]->TryGetObject(LItem) || LItem == nullptr)
		{
			return Fail(OutError, LPath, TEXT("不是对象"));
		}
		FDoodleCandidateReport LCandidate;
		if (!GetString(*LItem, TEXT("candidate_id"), LPath + TEXT(".candidate_id"), LCandidate.CandidateId, OutError)) return false;
		if (!GetEnum(*LItem, TEXT("status"), LPath + TEXT(".status"), LCandidate.Status, OutError, &DoodleRenderStatusFromString)) return false;
		GetInt(*LItem, TEXT("candidate_index"), LPath + TEXT(".candidate_index"), LCandidate.CandidateIndex, OutError, false);
		GetInt64(*LItem, TEXT("render_ms"), LPath + TEXT(".render_ms"), LCandidate.RenderMs, OutError, false);
		GetInt(*LItem, TEXT("light_count"), LPath + TEXT(".light_count"), LCandidate.LightCount, OutError, false);
		GetInt(*LItem, TEXT("frame_count"), LPath + TEXT(".frame_count"), LCandidate.FrameCount, OutError, false);
		TSharedPtr<FJsonObject> LMetrics;
		if (TryGetObjectFieldPtr(*LItem, TEXT("metrics"), LMetrics) && LMetrics.IsValid())
		{
			LCandidate.Metrics = LMetrics;
		}
		OutReport.Candidates.Add(MoveTemp(LCandidate));
	}
	return true;
}
