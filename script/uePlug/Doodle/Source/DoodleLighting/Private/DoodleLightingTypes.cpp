#include "DoodleLightingTypes.h"

/**
 * 枚举 ↔ JSON 串。
 *
 * 一张表同时给出两个方向，避免「加了一个枚举值只改了半边」——服务端 rig.h 的
 * NLOHMANN_JSON_SERIALIZE_ENUM 是逐字对齐的权威，改名 = 破协议（服务端 ORM 存的是名字）。
 *
 * 调用点的第三个实参就是初始化列表本身，**不要再套 `({...})`**：那是 GNU 语句表达式，
 * MSVC 在标准 C++ 下直接报 C2059。宏里 `__VA_ARGS__` 出现两次，所以每次展开都会把
 * 同一份列表再写一遍 —— 需要一个函数作用域的 `static` 变量，用 `(__VA_ARGS__)` 的
 * 圆括号包住即可保持「逗号不进宏实参切分」。
 */
#define DOODLE_LIGHTING_ENUM_IMPL(FnPrefix, EnumType, ...)                                     \
	FString FnPrefix##ToString(EnumType InValue)                                               \
	{                                                                                          \
		static const TMap<EnumType, const TCHAR*> LToName = __VA_ARGS__;                       \
		if (const TCHAR* const* LFound = LToName.Find(InValue))                                \
		{                                                                                      \
			return *LFound;                                                                    \
		}                                                                                      \
		return FString();                                                                      \
	}                                                                                          \
	bool FnPrefix##FromString(const FString& InText, EnumType& OutValue)                       \
	{                                                                                          \
		static const TMap<EnumType, const TCHAR*> LToName = __VA_ARGS__;                       \
		static const TMap<FString, EnumType> LFromName = []() {                                \
			TMap<FString, EnumType> LOut;                                                      \
			for (const TPair<EnumType, const TCHAR*>& LKv : LToName)                           \
			{                                                                                  \
				LOut.Add(FString(LKv.Value), LKv.Key);                                         \
			}                                                                                  \
			return LOut;                                                                       \
		}();                                                                                   \
		if (const EnumType* LFound = LFromName.Find(InText))                                   \
		{                                                                                      \
			OutValue = *LFound;                                                                \
			return true;                                                                       \
		}                                                                                      \
		return false;                                                                          \
	}

DOODLE_LIGHTING_ENUM_IMPL(
	DoodleLightRole, EDoodleLightRole,
	{
		{EDoodleLightRole::Key, TEXT("key")},
		{EDoodleLightRole::Fill, TEXT("fill")},
		{EDoodleLightRole::Rim, TEXT("rim")},
		{EDoodleLightRole::Ambient, TEXT("ambient")},
		{EDoodleLightRole::Practical, TEXT("practical")},
		{EDoodleLightRole::Bounce, TEXT("bounce")},
		{EDoodleLightRole::Fx, TEXT("fx")},
	}
)

DOODLE_LIGHTING_ENUM_IMPL(
	DoodleLightType, EDoodleLightType,
	{
		{EDoodleLightType::Point, TEXT("point")},
		{EDoodleLightType::Spot, TEXT("spot")},
		{EDoodleLightType::Rect, TEXT("rect")},
		{EDoodleLightType::Directional, TEXT("directional")},
		{EDoodleLightType::Sky, TEXT("sky")},
	}
)

DOODLE_LIGHTING_ENUM_IMPL(
	DoodleAnchorSpace, EDoodleAnchorSpace,
	{
		{EDoodleAnchorSpace::Camera, TEXT("camera")},
		{EDoodleAnchorSpace::World, TEXT("world")},
		{EDoodleAnchorSpace::Subject, TEXT("subject")},
	}
)

DOODLE_LIGHTING_ENUM_IMPL(
	DoodleIntensityUnit, EDoodleIntensityUnit,
	{
		{EDoodleIntensityUnit::Candela, TEXT("candela")},
		{EDoodleIntensityUnit::Lumen, TEXT("lumen")},
		{EDoodleIntensityUnit::Lux, TEXT("lux")},
		{EDoodleIntensityUnit::Unitless, TEXT("unitless")},
	}
)

DOODLE_LIGHTING_ENUM_IMPL(
	DoodleColorMode, EDoodleColorMode,
	{
		{EDoodleColorMode::Temperature, TEXT("temperature")},
		{EDoodleColorMode::RGB, TEXT("rgb")},
		{EDoodleColorMode::Filter, TEXT("filter")},
	}
)

DOODLE_LIGHTING_ENUM_IMPL(
	DoodleLightMobility, EDoodleLightMobility,
	{
		{EDoodleLightMobility::StaticLight, TEXT("static_light")},
		{EDoodleLightMobility::Stationary, TEXT("stationary")},
		{EDoodleLightMobility::Movable, TEXT("movable")},
	}
)

DOODLE_LIGHTING_ENUM_IMPL(
	DoodleLightOrigin, EDoodleLightOrigin,
	{
		{EDoodleLightOrigin::AI, TEXT("ai")},
		{EDoodleLightOrigin::Artist, TEXT("artist")},
		{EDoodleLightOrigin::Inherited, TEXT("inherited")},
	}
)

DOODLE_LIGHTING_ENUM_IMPL(
	DoodleVariantAxis, EDoodleVariantAxis,
	{
		{EDoodleVariantAxis::None, TEXT("none")},
		{EDoodleVariantAxis::KeyDirection, TEXT("key_direction")},
		{EDoodleVariantAxis::KeyFillRatio, TEXT("key_fill_ratio")},
		{EDoodleVariantAxis::Kelvin, TEXT("kelvin")},
		{EDoodleVariantAxis::Hardness, TEXT("hardness")},
		{EDoodleVariantAxis::AmbientLevel, TEXT("ambient_level")},
		{EDoodleVariantAxis::GradingStrength, TEXT("grading_strength")},
	}
)

DOODLE_LIGHTING_ENUM_IMPL(
	DoodleLightOpKind, EDoodleLightOpKind,
	{
		{EDoodleLightOpKind::Add, TEXT("add")},
		{EDoodleLightOpKind::Modify, TEXT("modify")},
		{EDoodleLightOpKind::Remove, TEXT("remove")},
		{EDoodleLightOpKind::SetGrading, TEXT("set_grading")},
		{EDoodleLightOpKind::SetEnvironment, TEXT("set_environment")},
		{EDoodleLightOpKind::SetExposure, TEXT("set_exposure")},
		{EDoodleLightOpKind::SetWhiteBalance, TEXT("set_white_balance")},
	}
)

DOODLE_LIGHTING_ENUM_IMPL(
	DoodleRenderStatus, EDoodleRenderStatus,
	{
		{EDoodleRenderStatus::Pending, TEXT("pending")},
		{EDoodleRenderStatus::Rendered, TEXT("rendered")},
		{EDoodleRenderStatus::Failed, TEXT("failed")},
	}
)

#undef DOODLE_LIGHTING_ENUM_IMPL
