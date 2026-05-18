#include "LyraExperienceDefinition.h"

#include "GameFeatureAction.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif


#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraExperienceDefinition)

#define LOCTEXT_NAMESPACE "LyraSystem"

ULyraExperienceDefinition::ULyraExperienceDefinition()
{
}

#if WITH_EDITOR
EDataValidationResult ULyraExperienceDefinition::IsDataValid(class FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);
	int32 EntryIndex = 0;
	for (const UGameFeatureAction* Action : Actions)
	{
		if (Action)
		{
			EDataValidationResult ChildResult = Action->IsDataValid(Context);
			Result = CombineDataValidationResults(Result, ChildResult);
		}
		else
		{
			Result = EDataValidationResult::Invalid;
			Context.AddError(FText::Format(LOCTEXT("ActionEntryIsNull", "Null entry at index {0} in Actions"), FText::AsNumber(EntryIndex)));
		}
		EntryIndex++;
	}

	if (!GetClass()->IsNative())
	{
		const UClass* ParentClass = GetClass()->GetSuperClass();
		const UClass* FirstNativeParent = ParentClass;
		/**
		 * 如果是多级蓝图继承的话，这样写可以找到蓝图应该继承的那个C++类，方便日志来输出这个C++类的名字
		 * 如果是直接用GetClass()->GetSuperClass()->IsNative()也能判断是不是多级蓝图继承，但是达不日志输出应该继承的那个C++类这种效果，
		 */
		while ((FirstNativeParent != nullptr) && !FirstNativeParent->IsNative())
		{
			FirstNativeParent = FirstNativeParent->GetSuperClass();
		}
		if (ParentClass != FirstNativeParent)
		{
			Context.AddError(FText::Format(LOCTEXT("ExperienceInheritanceIsUnsupported",
			                                       "Blueprint subclasses of Blueprint experiences is not currently supported (use composition via ActionSets instead). Parent class was {0} but should be {1}."),
			                               FText::AsCultureInvariant(GetPathNameSafe(ParentClass)),
			                               FText::AsCultureInvariant(GetPathNameSafe(FirstNativeParent))
			));
			Result = EDataValidationResult::Invalid;
		}
	}

	return Result;
}


#endif
#if WITH_EDITORONLY_DATA

void ULyraExperienceDefinition::UpdateAssetBundleData()
{
	Super::UpdateAssetBundleData();
	for (UGameFeatureAction* Action : Actions)
	{
		if (Action)
		{
			Action->AddAdditionalAssetBundleData(AssetBundleData);
		}
	}
}

#endif


#undef LOCTEXT_NAMESPACE
