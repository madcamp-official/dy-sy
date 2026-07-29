// Copyright (c) 2026 Kitsana Puengsri. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "Fonts/SlateFontInfo.h"
#include "Framework/Text/TextLayout.h"
#include "TextAnimTypes.h"
#include "AnimatedTextBlock.generated.h"

class SAnimatedText;
class UTextAnimPreset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTextAnimatorEvent);

/**
 * Text block with built-in per-character animations.
 * Pick an Effect (or a Preset), tune Params, call Activate() (or auto-activate).
 */
UCLASS(meta = (DisplayName = "Animated Text Block"))
class TEXTANIMATOR_API UAnimatedTextBlock : public UWidget
{
	GENERATED_BODY()

public:
	UAnimatedTextBlock();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Content", meta = (MultiLine = "true"))
	FText Text;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	FSlateFontInfo Font;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	FLinearColor ColorAndOpacity = FLinearColor::White;

	/** Drop shadow offset in pixels. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	FVector2D ShadowOffset = FVector2D(1.f, 1.f);

	/** Drop shadow color. Alpha 0 disables the shadow. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	FLinearColor ShadowColorAndOpacity = FLinearColor(0.f, 0.f, 0.f, 0.f);

	/** How each line is aligned inside the widget's width. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	TEnumAsByte<ETextJustify::Type> Justification = ETextJustify::Left;

	/** Force text to upper/lower case before display. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	ETextTransformPolicy TextTransformPolicy = ETextTransformPolicy::None;

	/** Minimum desired width regardless of text length. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance", meta = (ClampMin = "0"))
	float MinDesiredWidth = 0.f;

	/** Multiplier on line height for multi-line text. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance", meta = (ClampMin = "0.1"))
	float LineHeightPercentage = 1.f;

	/** Wrap text onto a new line when it reaches the widget's width. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wrapping")
	bool bAutoWrapText = false;

	/** Wrap at this fixed pixel width instead (0 = off). Takes priority over AutoWrapText. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wrapping", meta = (ClampMin = "0"))
	float WrapTextAt = 0.f;

	/** Optional preset that overrides Effect + Params when set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UTextAnimPreset> Preset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (EditCondition = "Preset == nullptr"))
	ETextAnimEffect Effect = ETextAnimEffect::Typewriter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (EditCondition = "Preset == nullptr"))
	FTextAnimParams Params;

	/** Start the animation automatically when the widget is constructed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	bool bAutoActivate = true;

	/** Play the animation in the UMG Designer too (replays whenever a property changes). */
	UPROPERTY(EditAnywhere, Category = "Animation")
	bool bPreviewInDesigner = true;

	/** Fired when the entrance animation finishes. */
	UPROPERTY(BlueprintAssignable, Category = "Text Animator")
	FTextAnimatorEvent OnAnimationFinished;

	/** Start (or restart) the entrance animation from the beginning. */
	UFUNCTION(BlueprintCallable, Category = "Text Animator")
	void Activate();

	/** Stop the animation and show the full text. */
	UFUNCTION(BlueprintCallable, Category = "Text Animator")
	void Deactivate();

	/** Jump to the fully-revealed end state instantly. */
	UFUNCTION(BlueprintCallable, Category = "Text Animator")
	void SkipToEnd();

	/** True while the animation is running. */
	UFUNCTION(BlueprintPure, Category = "Text Animator")
	bool IsActive() const;

	UFUNCTION(BlueprintCallable, Category = "Text Animator")
	void SetText(FText InText, bool bReplay = true);

	UFUNCTION(BlueprintCallable, Category = "Text Animator")
	void SetEffect(ETextAnimEffect InEffect, bool bReplay = true);

	UFUNCTION(BlueprintCallable, Category = "Text Animator")
	void SetParams(FTextAnimParams InParams);

	UFUNCTION(BlueprintCallable, Category = "Text Animator")
	void SetPreset(UTextAnimPreset* InPreset, bool bReplay = true);

	UFUNCTION(BlueprintCallable, Category = "Text Animator")
	void SetColorAndOpacity(FLinearColor InColor);

	UFUNCTION(BlueprintCallable, Category = "Text Animator")
	void SetShadowColorAndOpacity(FLinearColor InColor);

	UFUNCTION(BlueprintCallable, Category = "Text Animator")
	void SetJustification(TEnumAsByte<ETextJustify::Type> InJustification);

	// UWidget
	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	ETextAnimEffect GetEffectiveEffect() const;
	const FTextAnimParams& GetEffectiveParams() const;

	TSharedPtr<SAnimatedText> MyAnimatedText;
};
