// Fill out your copyright notice in the Description page of Project Settings.


#include "AbcWrap/DoodleGeometryCacheCurveAsset.h"

// Add default functionality here for any IDoodleGeometryCacheCurveAsset functions that are not pure virtual.
UDoodleGeometryCacheCurveAsset::UDoodleGeometryCacheCurveAsset(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer), materalName(), tranAnm() {

}

TArray<FTransformCurve>& UDoodleGeometryCacheCurveAsset::GetTranAnm()
{
	return tranAnm;
}
