#include "LyraSettingsLocal.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraSettingsLocal)

//////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
static TAutoConsoleVariable<bool> CVarApplyFrameRateSettingsInPIE(TEXT("Lyra.Settings.ApplyFrameRateSettingsInPIE"),
                                                                  false,
                                                                  TEXT("是否在 PIE 中启用 PC 风格的帧率限制逻辑？"),
                                                                  ECVF_Default);

static TAutoConsoleVariable<bool> CVarApplyFrontEndPerformanceOptionsInPIE(TEXT("Lyra.Settings.ApplyFrontEndPerformanceOptionsInPIE"),
                                                                           false,
                                                                           TEXT("是否在 PIE 中启用前端菜单专用性能设置？"),
                                                                           ECVF_Default);

static TAutoConsoleVariable<bool> CVarApplyDeviceProfilesInPIE(TEXT("Lyra.Settings.ApplyDeviceProfilesInPIE"),
                                                               false,
                                                               TEXT("是否在 PIE 中应用模拟平台 / Experience 驱动的 DeviceProfile？"),
                                                               ECVF_Default);
#endif

//////////////////////////////////////////////////////////////////////

ULyraSettingsLocal::ULyraSettingsLocal()
{
}

ULyraSettingsLocal* ULyraSettingsLocal::Get()
{
	return GEngine ? CastChecked<ULyraSettingsLocal>(GEngine->GetGameUserSettings()) : nullptr;
}
