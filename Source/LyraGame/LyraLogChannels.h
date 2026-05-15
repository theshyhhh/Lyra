#pragma once

#include "Logging/LogMacros.h"

class UObject;

LYRAGAME_API DECLARE_LOG_CATEGORY_EXTERN(LogLyra, Log, All);
LYRAGAME_API DECLARE_LOG_CATEGORY_EXTERN(LogLyraExperience, Log, All);
LYRAGAME_API DECLARE_LOG_CATEGORY_EXTERN(LogLyraAbilitySystem, Log, All);
LYRAGAME_API DECLARE_LOG_CATEGORY_EXTERN(LogLyraTeams, Log, All);

/**
 * GetClientServerContextString 是一个日志辅助函数，作用是给 Lyra 的日志补上“这条日志来自服务端还是客户端”的上下文。
 * @param ContextObject 上下文对象
 * @return 返回这条日志来自服务端还是客户端
 */
LYRAGAME_API FString GetClientServerContextString(UObject* ContextObject = nullptr);
