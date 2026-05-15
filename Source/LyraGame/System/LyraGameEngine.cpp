#include "LyraGameEngine.h"

//把 UHT 生成的 xxx.gen.cpp 内联进当前 .cpp 编译单元，减少 generated cpp 单独编译时重复解析头文件的开销，从而优化  C++ 项目的编译速度。
#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraGameEngine)

ULyraGameEngine::ULyraGameEngine(const FObjectInitializer& ObjectInitializer)
{
}

void ULyraGameEngine::Init(IEngineLoop* InEngineLoop)
{
	Super::Init(InEngineLoop);
}
