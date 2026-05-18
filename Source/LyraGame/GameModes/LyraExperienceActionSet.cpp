#include "LyraExperienceActionSet.h"

#include "GameFeatureAction.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraExperienceActionSet)

#define LOCTEXT_NAMESPACE "LyraSystem"
#if WITH_EDITOR
ULyraExperienceActionSet::ULyraExperienceActionSet()
{
}

EDataValidationResult ULyraExperienceActionSet::IsDataValid(class FDataValidationContext& Context) const
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

		++EntryIndex;
	}

	return Result;
}
#endif

#if WITH_EDITORONLY_DATA
void ULyraExperienceActionSet::UpdateAssetBundleData()
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
