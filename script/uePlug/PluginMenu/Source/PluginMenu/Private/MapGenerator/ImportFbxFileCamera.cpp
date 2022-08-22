#include "MapGenerator/ImportFbxFileCamera.h"

#include "LevelSequence/Public/LevelSequence.h"
#include "MovieScene.h"
#include "Sequencer/Public/ISequencer.h"
#include "FbxImporter.h"
#include "CoreUObject/Public/UObject/UObjectGlobals.h"
#include "Runtime/Core/Public/Math/UnitConversion.h"
#include "Editor.h"

#include "Editor/MovieSceneTools/Public/MatineeImportTools.h"
#include "Editor/MovieSceneTools/Public/MovieSceneToolsUserSettings.h"
#include "Editor/MovieSceneTools/Public/MovieSceneToolsProjectSettings.h"

#include "fbxsdk/scene/geometry/fbxcameraswitcher.h"
#include "fbxsdk/scene/geometry/fbxcamera.h"
#include "fbxsdk/scene/animation/fbxanimstack.h"
#include "fbxsdk/scene/animation/fbxanimlayer.h"
#include "fbxsdk/scene/geometry/fbxnode.h"
#include "fbxsdk/scene/animation/fbxanimcurve.h"

#include "MovieSceneTracks/Public/Tracks/MovieSceneCameraCutTrack.h"
#include "MovieSceneTracks/Public/Tracks/MovieSceneFloatTrack.h"
#include "MovieScene/Public/Channels/MovieSceneFloatChannel.h"
#include "MovieSceneTracks/Public/Sections/MovieSceneFloatSection.h"
#include "Tracks/MovieScene3DTransformTrack.h"
#include "Sections/MovieScene3DTransformSection.h"
#include "MovieScene/Public/Channels/MovieSceneChannelProxy.h"

#include "CinematicCamera/Public/CineCameraActor.h"
#include "CinematicCamera/Public/CineCameraComponent.h"
#include "Channels/MovieSceneChannelTraits.h"

#ifdef LOCTEXT_NAMESPACE
#undef LOCTEXT_NAMESPACE
#endif

#define LOCTEXT_NAMESPACE "FImportFbxFileCamera"

bool ImportFBXProperty(FString NodeName, FString AnimatedPropertyName, FGuid ObjectBinding, UnFbx::FFbxCurvesAPI &CurveAPI, UMovieScene *InMovieScene, ISequencer &InSequencer)
{
	const UMovieSceneToolsProjectSettings *ProjectSettings = GetDefault<UMovieSceneToolsProjectSettings>();
	const UMovieSceneUserImportFBXSettings *ImportFBXSettings = GetDefault<UMovieSceneUserImportFBXSettings>();

	TArrayView<TWeakObjectPtr<>> BoundObjects = InSequencer.FindBoundObjects(ObjectBinding, InSequencer.GetFocusedTemplateID());

	for (auto FbxSetting : ProjectSettings->FbxSettings)
	{
		if (FCString::Strcmp(*FbxSetting.FbxPropertyName.ToUpper(), *AnimatedPropertyName.ToUpper()) != 0)
		{
			continue;
		}

		for (TWeakObjectPtr<> &WeakObject : BoundObjects)
		{
			UObject *FoundObject = WeakObject.Get();

			if (!FoundObject)
			{
				continue;
			}

			UObject *PropertyOwner = FoundObject;
			if (!FbxSetting.PropertyPath.ComponentName.IsEmpty())
			{
				PropertyOwner = FindObjectFast<UObject>(FoundObject, *FbxSetting.PropertyPath.ComponentName);
			}

			if (!PropertyOwner)
			{
				continue;
			}

			FGuid PropertyOwnerGuid = InSequencer.GetHandleToObject(PropertyOwner);
			if (!PropertyOwnerGuid.IsValid())
			{
				continue;
			}

			UMovieSceneFloatTrack *FloatTrack = InMovieScene->FindTrack<UMovieSceneFloatTrack>(PropertyOwnerGuid, *FbxSetting.PropertyPath.PropertyName);
			if (!FloatTrack)
			{
				InMovieScene->Modify();
				FloatTrack = InMovieScene->AddTrack<UMovieSceneFloatTrack>(PropertyOwnerGuid);
				FloatTrack->SetPropertyNameAndPath(*FbxSetting.PropertyPath.PropertyName, *FbxSetting.PropertyPath.PropertyName);
			}

			if (FloatTrack)
			{
				FloatTrack->RemoveAllAnimationData();

				FFrameRate FrameRate = FloatTrack->GetTypedOuter<UMovieScene>()->GetTickResolution();

				bool bSectionAdded = false;
				UMovieSceneFloatSection *FloatSection = Cast<UMovieSceneFloatSection>(FloatTrack->FindOrAddSection(0, bSectionAdded));
				if (!FloatSection)
				{
					continue;
				}

				FloatSection->Modify();

				if (bSectionAdded)
				{
					FloatSection->SetRange(TRange<FFrameNumber>::All());
				}

				const int32 ChannelIndex = 0;
				const int32 CompositeIndex = 0;
				FRichCurve CurveHandle;
				const bool bNegative = false;
				CurveAPI.GetCurveData(NodeName, AnimatedPropertyName, ChannelIndex, CompositeIndex, CurveHandle, bNegative);

				FMovieSceneFloatChannel *Channel = FloatSection->GetChannelProxy().GetChannel<FMovieSceneFloatChannel>(0);
				TMovieSceneChannelData<FMovieSceneFloatValue> ChannelData = Channel->GetData();

				double DecimalRate = FrameRate.AsDecimal();
				ChannelData.Reset();

				for (auto L_It = CurveHandle.GetKeyHandleIterator(); L_It; ++L_It)
				{
					FKeyHandle L_KeyHandle = *L_It;
					FRichCurveKey &L_Key = CurveHandle.GetKey(L_KeyHandle);
					float ArriveTangent = L_Key.ArriveTangent;
					float LeaveTangent = L_Key.LeaveTangent;
					FFrameNumber KeyTime = (CurveHandle.GetKeyTime(L_KeyHandle) * FrameRate).RoundToFrame();
					float L_Value = CurveHandle.GetKeyValue(L_KeyHandle);

					EInterpCurveMode InterpMode{};
					switch (CurveHandle.GetKeyInterpMode(L_KeyHandle))
					{
					case ERichCurveInterpMode::RCIM_Constant:
						InterpMode = EInterpCurveMode::CIM_Constant;
						break;
					case ERichCurveInterpMode::RCIM_Cubic:
						InterpMode = EInterpCurveMode::CIM_CurveAuto;
						break;
					case ERichCurveInterpMode::RCIM_Linear:
						InterpMode = EInterpCurveMode::CIM_Linear;
						break;
					case ERichCurveInterpMode::RCIM_None:
						InterpMode = EInterpCurveMode::CIM_Unknown;
						break;
					default:
						InterpMode = EInterpCurveMode::CIM_CurveAuto;
						break;
					}
					FMatineeImportTools::SetOrAddKey(ChannelData,
													 KeyTime,
													 L_Value, // CurveHandle.Keys[KeyIndex].OutVal,
													 ArriveTangent,
													 LeaveTangent,
													 InterpMode, //  CurveHandle.Keys[KeyIndex].InterpMode,
													 FrameRate);
				}

				if (ImportFBXSettings->bReduceKeys)
				{
					FKeyDataOptimizationParams Params;
					Params.Tolerance = ImportFBXSettings->ReduceKeysTolerance;
					::UE::MovieScene::Optimize(Channel, Params);
				}
				Channel->AutoSetTangents();

				return true;
			}
		}
	}
	return false;
}

void ImportTransformChannel(const FRichCurve &Source, FMovieSceneFloatChannel *Dest, FFrameRate DestFrameRate, bool bNegateTangents)
{
	TMovieSceneChannelData<FMovieSceneFloatValue> ChannelData = Dest->GetData();
	ChannelData.Reset();
	double DecimalRate = DestFrameRate.AsDecimal();

	for (auto L_It = Source.GetKeyHandleIterator(); L_It; ++L_It)
	{
		FKeyHandle L_KeyHandle = *L_It;
		FRichCurveKey L_Key = Source.GetKey(L_KeyHandle);
		float ArriveTangent = L_Key.ArriveTangent;
		float LeaveTangent = L_Key.LeaveTangent;
		FFrameNumber KeyTime = (Source.GetKeyTime(L_KeyHandle) * DestFrameRate).RoundToFrame();
		float L_Value = Source.GetKeyValue(L_KeyHandle);

		EInterpCurveMode InterpMode{};
		switch (Source.GetKeyInterpMode(L_KeyHandle))
		{
		case ERichCurveInterpMode::RCIM_Constant:
			InterpMode = EInterpCurveMode::CIM_Constant;
			break;
		case ERichCurveInterpMode::RCIM_Cubic:
			InterpMode = EInterpCurveMode::CIM_CurveAuto;
			break;
		case ERichCurveInterpMode::RCIM_Linear:
			InterpMode = EInterpCurveMode::CIM_Linear;
			break;
		case ERichCurveInterpMode::RCIM_None:
			InterpMode = EInterpCurveMode::CIM_Unknown;
			break;
		default:
			InterpMode = EInterpCurveMode::CIM_CurveAuto;
			break;
		}
		FMatineeImportTools::SetOrAddKey(ChannelData,
										 KeyTime,
										 L_Value, // Source.Keys[KeyIndex].OutVal,
										 ArriveTangent,
										 LeaveTangent,
										 InterpMode, //  Source.Keys[KeyIndex].InterpMode,
										 DestFrameRate);
	}

	const UMovieSceneUserImportFBXSettings *ImportFBXSettings = GetDefault<UMovieSceneUserImportFBXSettings>();
	if (ImportFBXSettings->bReduceKeys)
	{
		FKeyDataOptimizationParams Params;
		Params.Tolerance = ImportFBXSettings->ReduceKeysTolerance;
		UE::MovieScene::Optimize(Dest, Params);
	}
	Dest->AutoSetTangents();
}

bool ImportFBXTransform(FString NodeName, FGuid ObjectBinding, UnFbx::FFbxCurvesAPI &CurveAPI, UMovieScene *InMovieScene)
{
	const UMovieSceneUserImportFBXSettings *ImportFBXSettings = GetDefault<UMovieSceneUserImportFBXSettings>();

	// Look for transforms explicitly
	FRichCurve Translation[3];
	FRichCurve EulerRotation[3];
	FRichCurve Scale[3];
	FTransform DefaultTransform;
	CurveAPI.GetConvertedTransformCurveData(NodeName,
											Translation[0], Translation[1], Translation[2],
											EulerRotation[0], EulerRotation[1], EulerRotation[2],
											Scale[0], Scale[1], Scale[2],

											DefaultTransform, true, 1.0f);

	UMovieScene3DTransformTrack *TransformTrack = InMovieScene->FindTrack<UMovieScene3DTransformTrack>(ObjectBinding);
	if (!TransformTrack)
	{
		InMovieScene->Modify();
		TransformTrack = InMovieScene->AddTrack<UMovieScene3DTransformTrack>(ObjectBinding);
	}
	TransformTrack->RemoveAllAnimationData();

	bool bSectionAdded = false;
	UMovieScene3DTransformSection *TransformSection = Cast<UMovieScene3DTransformSection>(TransformTrack->FindOrAddSection(0, bSectionAdded));
	if (!TransformSection)
	{
		return false;
	}

	TransformSection->Modify();

	FFrameRate FrameRate = TransformSection->GetTypedOuter<UMovieScene>()->GetTickResolution();

	if (bSectionAdded)
	{
		TransformSection->SetRange(TRange<FFrameNumber>::All());
	}

	FVector Location = DefaultTransform.GetLocation(), Rotation = DefaultTransform.GetRotation().Euler(), Scale3D = DefaultTransform.GetScale3D();

	TArrayView<FMovieSceneFloatChannel *> Channels = TransformSection->GetChannelProxy().GetChannels<FMovieSceneFloatChannel>();

	Channels[0]->SetDefault(Location.X);
	Channels[1]->SetDefault(Location.Y);
	Channels[2]->SetDefault(Location.Z);

	Channels[3]->SetDefault(Rotation.X);
	Channels[4]->SetDefault(Rotation.Y);
	Channels[5]->SetDefault(Rotation.Z);

	Channels[6]->SetDefault(Scale3D.X);
	Channels[7]->SetDefault(Scale3D.Y);
	Channels[8]->SetDefault(Scale3D.Z);

	ImportTransformChannel(Translation[0], Channels[0], FrameRate, false);
	ImportTransformChannel(Translation[1], Channels[1], FrameRate, true);
	ImportTransformChannel(Translation[2], Channels[2], FrameRate, false);

	ImportTransformChannel(EulerRotation[0], Channels[3], FrameRate, false);
	ImportTransformChannel(EulerRotation[1], Channels[4], FrameRate, true);
	ImportTransformChannel(EulerRotation[2], Channels[5], FrameRate, true);

	ImportTransformChannel(Scale[0], Channels[6], FrameRate, false);
	ImportTransformChannel(Scale[1], Channels[7], FrameRate, false);
	ImportTransformChannel(Scale[2], Channels[8], FrameRate, false);

	return true;
}

bool ImportFBXNode(FString NodeName, UnFbx::FFbxCurvesAPI &CurveAPI, UMovieScene *InMovieScene, ISequencer &InSequencer, const TMap<FGuid, FString> &InObjectBindingMap, bool bMatchByNameOnly)
{
	// Find the matching object binding to apply this animation to. If not matching by name only, default to the first.
	FGuid ObjectBinding;
	for (auto It = InObjectBindingMap.CreateConstIterator(); It; ++It)
	{
		if (!bMatchByNameOnly || FCString::Strcmp(*It.Value().ToUpper(), *NodeName.ToUpper()) == 0)
		{
			ObjectBinding = It.Key();
			break;
		}
	}

	if (!ObjectBinding.IsValid())
	{
		UE_LOG(LogMovieScene, Warning, TEXT("Fbx Import: Failed to find any matching node for (%s)."), *NodeName);
		return false;
	}

	// Look for animated float properties
	TArray<FString> AnimatedPropertyNames;
	CurveAPI.GetNodeAnimatedPropertyNameArray(NodeName, AnimatedPropertyNames);

	for (auto AnimatedPropertyName : AnimatedPropertyNames)
	{
		ImportFBXProperty(NodeName, AnimatedPropertyName, ObjectBinding, CurveAPI, InMovieScene, InSequencer);
	}

	ImportFBXTransform(NodeName, ObjectBinding, CurveAPI, InMovieScene);

	return true;
}

void GetCameras(FbxNode *Parent, TArray<FbxCamera *> &Cameras)
{
	FbxCamera *Camera = Parent->GetCamera();
	if (Camera)
	{
		Cameras.Add(Camera);
	}

	int32 NodeCount = Parent->GetChildCount();
	for (int32 NodeIndex = 0; NodeIndex < NodeCount; ++NodeIndex)
	{
		FbxNode *Child = Parent->GetChild(NodeIndex);
		GetCameras(Child, Cameras);
	}
}

FbxCamera *FindCamera(FbxNode *Parent)
{
	FbxCamera *Camera = Parent->GetCamera();
	if (!Camera)
	{
		int32 NodeCount = Parent->GetChildCount();
		for (int32 NodeIndex = 0; NodeIndex < NodeCount && !Camera; ++NodeIndex)
		{
			FbxNode *Child = Parent->GetChild(NodeIndex);
			Camera = Child->GetCamera();
		}
	}

	return Camera;
}

FbxNode *RetrieveObjectFromName(const TCHAR *ObjectName, FbxNode *Root)
{
	if (!Root)
	{
		return nullptr;
	}

	for (int32 ChildIndex = 0; ChildIndex < Root->GetChildCount(); ++ChildIndex)
	{
		FbxNode *Node = Root->GetChild(ChildIndex);
		if (Node)
		{
			FString NodeName = FString(Node->GetName());

			if (!FCString::Strcmp(ObjectName, UTF8_TO_TCHAR(Node->GetName())))
			{
				return Node;
			}

			if (FbxNode *NextNode = RetrieveObjectFromName(ObjectName, Node))
			{
				return NextNode;
			}
		}
	}

	return nullptr;
}

void CopyCameraProperties(FbxCamera *CameraNode, ACineCameraActor *CameraActor)
{
	float FieldOfView;
	float FocalLength;

	if (CameraNode->GetApertureMode() == FbxCamera::eFocalLength)
	{
		FocalLength = CameraNode->FocalLength.Get();
		FieldOfView = CameraNode->ComputeFieldOfView(FocalLength);
	}
	else
	{
		FieldOfView = CameraNode->FieldOfView.Get();
		FocalLength = CameraNode->ComputeFocalLength(FieldOfView);
	}

	float ApertureWidth = CameraNode->GetApertureWidth();
	float ApertureHeight = CameraNode->GetApertureHeight();

	UCineCameraComponent *CineCameraComponent = CameraActor->GetCineCameraComponent();

	CineCameraComponent->SetProjectionMode(CameraNode->ProjectionType.Get() == FbxCamera::ePerspective ? ECameraProjectionMode::Perspective : ECameraProjectionMode::Orthographic);
	CineCameraComponent->SetAspectRatio(CameraNode->AspectWidth.Get() / CameraNode->AspectHeight.Get());
	CineCameraComponent->SetOrthoNearClipPlane(CameraNode->NearPlane.Get());
	CineCameraComponent->SetOrthoFarClipPlane(CameraNode->FarPlane.Get());
	CineCameraComponent->SetOrthoWidth(CameraNode->OrthoZoom.Get());
	CineCameraComponent->SetFieldOfView(FieldOfView);
	CineCameraComponent->Filmback.SensorWidth = FUnitConversion::Convert(ApertureWidth, EUnit::Inches, EUnit::Millimeters);
	CineCameraComponent->Filmback.SensorHeight = FUnitConversion::Convert(ApertureHeight, EUnit::Inches, EUnit::Millimeters);
	CineCameraComponent->SetFilmbackPresetByName(TEXT("16:9 DSLR"));
	CineCameraComponent->FocusSettings.FocusMethod = ECameraFocusMethod::Disable;

	if (FocalLength < CineCameraComponent->LensSettings.MinFocalLength)
	{
		CineCameraComponent->LensSettings.MinFocalLength = FocalLength;
	}
	if (FocalLength > CineCameraComponent->LensSettings.MaxFocalLength)
	{
		CineCameraComponent->LensSettings.MaxFocalLength = FocalLength;
	}
	CineCameraComponent->CurrentFocalLength = FocalLength;
}

bool ImportFBXCamera(UnFbx::FFbxImporter *FbxImporter, UMovieScene *InMovieScene, ISequencer &InSequencer, TMap<FGuid, FString> &InObjectBindingMap, bool bMatchByNameOnly, bool bCreateCameras)
{
	bool bCamsCreated = false;
	if (bCreateCameras)
	{
		TArray<FbxCamera *> AllCameras;
		GetCameras(FbxImporter->Scene->GetRootNode(), AllCameras);

		// Find unmatched cameras
		TArray<FbxCamera *> UnmatchedCameras;
		for (auto Camera : AllCameras)
		{
			FString NodeName = FString(Camera->GetName());

			bool bMatched = false;
			for (auto InObjectBinding : InObjectBindingMap)
			{
				FString ObjectName = InObjectBinding.Value;
				if (!FCString::Strcmp(*ObjectName, UTF8_TO_TCHAR(Camera->GetName())))
				{
					bMatched = true;
					break;
				}
			}

			if (!bMatched)
			{
				UnmatchedCameras.Add(Camera);
			}
		}

		// Add any unmatched cameras
		// UWorld* World = GCurrentLevelEditingViewportClient ? GCurrentLevelEditingViewportClient->GetWorld() : nullptr;
		UWorld *World = GWorld;

		// If there are new cameras, clear the object binding map so that we're only assigning values to the newly created cameras
		if (UnmatchedCameras.Num() != 0)
		{
			InObjectBindingMap.Reset();
			bCamsCreated = true;
		}

		for (auto UnmatchedCamera : UnmatchedCameras)
		{
			FString CameraName = FString(ANSI_TO_TCHAR(UnmatchedCamera->GetName()));

			FActorSpawnParameters SpawnParams;
			SpawnParams.Name = *CameraName;
			ACineCameraActor *NewCamera = World->SpawnActor<ACineCameraActor>(SpawnParams);
			NewCamera->SetActorLabel(*CameraName);

			// Copy camera properties before adding default tracks so that initial camera properties match and can be restored after sequencer finishes
			CopyCameraProperties(UnmatchedCamera, NewCamera);

			TArray<TWeakObjectPtr<AActor>> NewCameras;
			NewCameras.Add(NewCamera);
			TArray<FGuid> NewCameraGuids = InSequencer.AddActors(NewCameras);

			if (NewCameraGuids.Num())
			{
				InObjectBindingMap.Add(NewCameraGuids[0]);
				InObjectBindingMap[NewCameraGuids[0]] = CameraName;
			}
		}
	}

	for (auto InObjectBinding : InObjectBindingMap)
	{
		TArrayView<TWeakObjectPtr<>> BoundObjects = InSequencer.FindBoundObjects(InObjectBinding.Key, InSequencer.GetFocusedTemplateID());

		FString ObjectName = InObjectBinding.Value;
		FbxCamera *CameraNode = nullptr;
		FbxNode *Node = RetrieveObjectFromName(*ObjectName, FbxImporter->Scene->GetRootNode());
		if (Node)
		{
			CameraNode = FindCamera(Node);
		}

		if (!CameraNode)
		{
			if (bMatchByNameOnly)
			{
				UE_LOG(LogMovieScene, Error, TEXT("Fbx Import: Failed to find any matching camera for (%s)."), *ObjectName);
				continue;
			}

			CameraNode = FindCamera(FbxImporter->Scene->GetRootNode());
			if (CameraNode)
			{
				UE_LOG(LogMovieScene, Warning, TEXT("Fbx Import: Failed to find exact matching camera for (%s). Using first camera from fbx (%s)"), *ObjectName, UTF8_TO_TCHAR(CameraNode->GetName()));
			}
		}

		if (!CameraNode)
		{
			continue;
		}

		float FieldOfView;
		float FocalLength;

		if (CameraNode->GetApertureMode() == FbxCamera::eFocalLength)
		{
			FocalLength = CameraNode->FocalLength.Get();
			FieldOfView = CameraNode->ComputeFieldOfView(FocalLength);
		}
		else
		{
			FieldOfView = CameraNode->FieldOfView.Get();
			FocalLength = CameraNode->ComputeFocalLength(FieldOfView);
		}

		for (TWeakObjectPtr<> &WeakObject : BoundObjects)
		{
			UObject *FoundObject = WeakObject.Get();
			if (FoundObject && FoundObject->IsA(ACineCameraActor::StaticClass()))
			{
				ACineCameraActor *CineCameraActor = Cast<ACineCameraActor>(FoundObject);
				UCineCameraComponent *CineCameraComponent = CineCameraActor->GetCineCameraComponent();
				CopyCameraProperties(CameraNode, CineCameraActor);

				// Set the default value of the current focal length section
				FGuid PropertyOwnerGuid = InSequencer.GetHandleToObject(CineCameraComponent);
				if (!PropertyOwnerGuid.IsValid())
				{
					continue;
				}

				UMovieSceneFloatTrack *FloatTrack = InMovieScene->FindTrack<UMovieSceneFloatTrack>(PropertyOwnerGuid, TEXT("CurrentFocalLength"));
				if (FloatTrack)
				{
					FloatTrack->RemoveAllAnimationData();

					bool bSectionAdded = false;
					UMovieSceneFloatSection *FloatSection = Cast<UMovieSceneFloatSection>(FloatTrack->FindOrAddSection(0, bSectionAdded));
					if (!FloatSection)
					{
						continue;
					}

					FloatSection->Modify();

					if (bSectionAdded)
					{
						FloatSection->SetRange(TRange<FFrameNumber>::All());
					}

					FloatSection->GetChannelProxy().GetChannel<FMovieSceneFloatChannel>(0)->SetDefault(FocalLength);
				}
			}
		}
	}
	return bCamsCreated;
}

FGuid FindCameraGuid(FbxCamera *Camera, TMap<FGuid, FString> &InObjectBindingMap)
{
	for (auto &Pair : InObjectBindingMap)
	{
		if (FCString::Strcmp(*Pair.Value, UTF8_TO_TCHAR(Camera->GetName())) == 0)
		{
			return Pair.Key;
		}
	}
	return FGuid();
}

UMovieSceneCameraCutTrack *GetCameraCutTrack(UMovieScene *InMovieScene)
{
	// Get the camera cut
	UMovieSceneTrack *CameraCutTrack = InMovieScene->GetCameraCutTrack();
	if (CameraCutTrack == nullptr)
	{
		InMovieScene->Modify();
		CameraCutTrack = InMovieScene->AddCameraCutTrack(UMovieSceneCameraCutTrack::StaticClass());
	}
	return CastChecked<UMovieSceneCameraCutTrack>(CameraCutTrack);
}

void ImportCameraCut(UnFbx::FFbxImporter *FbxImporter, UMovieScene *InMovieScene, ISequencer &InSequencer, TMap<FGuid, FString> &InObjectBindingMap)
{
	// Find a camera switcher
	FbxCameraSwitcher *CameraSwitcher = FbxImporter->Scene->GlobalCameraSettings().GetCameraSwitcher();
	if (CameraSwitcher == nullptr)
	{
		return;
	}
	// Get the animation layer
	FbxAnimStack *AnimStack = FbxImporter->Scene->GetMember<FbxAnimStack>(0);
	if (AnimStack == nullptr)
	{
		return;
	}
	FbxAnimLayer *AnimLayer = AnimStack->GetMember<FbxAnimLayer>(0);
	if (AnimLayer == nullptr)
	{
		return;
	}

	// The camera switcher camera index refer to depth-first found order of the camera in the FBX
	TArray<FbxCamera *> AllCameras;
	GetCameras(FbxImporter->Scene->GetRootNode(), AllCameras);

	UMovieSceneCameraCutTrack *CameraCutTrack = GetCameraCutTrack(InMovieScene);
	FFrameRate FrameRate = CameraCutTrack->GetTypedOuter<UMovieScene>()->GetTickResolution();

	FbxAnimCurve *AnimCurve = CameraSwitcher->CameraIndex.GetCurve(AnimLayer);
	if (AnimCurve)
	{
		for (int i = 0; i < AnimCurve->KeyGetCount(); ++i)
		{
			FbxAnimCurveKey key = AnimCurve->KeyGet(i);
			int value = (int)key.GetValue() - 1;
			if (value >= 0 && value < AllCameras.Num())
			{
				FGuid CameraGuid = FindCameraGuid(AllCameras[value], InObjectBindingMap);
				if (CameraGuid != FGuid())
				{
					CameraCutTrack->AddNewCameraCut(
						FMovieSceneObjectBindingID{
							UE::MovieScene::FFixedObjectBindingID{
								CameraGuid,
								MovieSceneSequenceID::Root}},
						(key.GetTime().GetSecondDouble() * FrameRate).RoundToFrame());
				}
			}
		}
		InSequencer.NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
	}
}

bool FImportFbxFileCamera::ImportCameraFromFbxFile(FString &ImportFilename, UMovieScene *MovieScene, ISequencer *Sequencer, TMap<FGuid, FString> &ObjectBindingMap)
{
	UMovieSceneUserImportFBXSettings *ImportFBXSettings = GetMutableDefault<UMovieSceneUserImportFBXSettings>();

	UnFbx::FFbxImporter *FbxImporter = UnFbx::FFbxImporter::GetInstance();

	UnFbx::FBXImportOptions *ImportOptions = FbxImporter->GetImportOptions();
	bool bConvertSceneBackup = ImportOptions->bConvertScene;
	bool bConvertSceneUnitBackup = ImportOptions->bConvertSceneUnit;
	bool bForceFrontXAxisBackup = ImportOptions->bForceFrontXAxis;

	ImportOptions->bConvertScene = true;
	ImportOptions->bConvertSceneUnit = true;
	ImportOptions->bForceFrontXAxis = ImportFBXSettings->bForceFrontXAxis;

	const FString FileExtension = FPaths::GetExtension(ImportFilename);
	if (!FbxImporter->ImportFromFile(*ImportFilename, FileExtension, true))
	{
		// Log the error message and fail the import.
		FbxImporter->ReleaseScene();
		ImportOptions->bConvertScene = bConvertSceneBackup;
		ImportOptions->bConvertSceneUnit = bConvertSceneUnitBackup;
		ImportOptions->bForceFrontXAxis = bForceFrontXAxisBackup;
		return false;
	}

	ImportFBXSettings->bMatchByNameOnly = false;
	ImportFBXSettings->bCreateCameras = true;
	ImportFBXSettings->bReduceKeys = false;

	const bool bMatchByNameOnly = ImportFBXSettings->bMatchByNameOnly;
	TOptional<bool> bCreateCameras;

	bool bCamsCreated = ImportFBXCamera(FbxImporter, MovieScene, *Sequencer, ObjectBindingMap, false, true);

	UnFbx::FFbxCurvesAPI CurveAPI;
	FbxImporter->PopulateAnimatedCurveData(CurveAPI);
	TArray<FString> AllNodeNames;
	CurveAPI.GetAllNodeNameArray(AllNodeNames);

	// Import a camera cut track if cams were created, do it after populating curve data ensure only one animation layer, if any
	if (bCamsCreated)
	{
		ImportCameraCut(FbxImporter, MovieScene, *Sequencer, ObjectBindingMap);
	}

	for (FString NodeName : AllNodeNames)
	{
		ImportFBXNode(NodeName, CurveAPI, MovieScene, *Sequencer, ObjectBindingMap, false);
	}

	Sequencer->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);

	FbxImporter->ReleaseScene();
	ImportOptions->bConvertScene = bConvertSceneBackup;
	ImportOptions->bConvertSceneUnit = bConvertSceneUnitBackup;
	ImportOptions->bForceFrontXAxis = bForceFrontXAxisBackup;

	return true;
}

#undef LOCTEXT_NAMESPACE