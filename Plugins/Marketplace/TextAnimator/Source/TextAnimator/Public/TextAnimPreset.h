// Copyright (c) 2026 Kitsana Puengsri. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TextAnimTypes.h"
#include "TextAnimPreset.generated.h"

/** Shareable animation preset. Assign to an Animated Text Block's Preset slot. */
UCLASS(BlueprintType)
class TEXTANIMATOR_API UTextAnimPreset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Preset")
	ETextAnimEffect Effect = ETextAnimEffect::Typewriter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Preset")
	FTextAnimParams Params;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override
	{
		// Keep the hidden mirror in sync so per-effect EditConditions evaluate correctly.
		Params.Effect = Effect;
		Super::PostEditChangeProperty(PropertyChangedEvent);
	}
#endif
};
