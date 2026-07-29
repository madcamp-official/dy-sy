// Copyright (c) 2026 Kitsana Puengsri. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "TextAnimTypes.generated.h"

UENUM(BlueprintType)
enum class ETextAnimEffect : uint8
{
	None		UMETA(DisplayName = "None (static)"),
	Typewriter	UMETA(DisplayName = "Typewriter"),
	FadeIn		UMETA(DisplayName = "Fade In"),
	SlideUp		UMETA(DisplayName = "Slide Up"),
	Wave		UMETA(DisplayName = "Wave")
};

UENUM(BlueprintType)
enum class ETextAnimEase : uint8
{
	Linear,
	EaseOut,
	EaseIn,
	EaseInOut,
	ElasticOut,
	BackOut,
	BounceOut
};

UENUM(BlueprintType)
enum class ETextAnimTimingMode : uint8
{
	PerCharacter	UMETA(DisplayName = "Per Character"),
	FixedDuration	UMETA(DisplayName = "Fixed Duration")
};

USTRUCT(BlueprintType)
struct FTextAnimParams
{
	GENERATED_BODY()

	/**
	 * Mirror of the owning widget's Effect, kept in sync by UAnimatedTextBlock.
	 * Only exists so the EditConditions below can show/hide params per effect.
	 */
	UPROPERTY()
	ETextAnimEffect Effect = ETextAnimEffect::Typewriter;

	/** Seconds each character takes to finish its own animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim", meta = (ClampMin = "0.01",
		EditCondition = "Effect == ETextAnimEffect::FadeIn || Effect == ETextAnimEffect::SlideUp",
		EditConditionHides))
	float CharAnimDuration = 0.4f;

	/** Whether characters stagger by a fixed per-character delay, or the whole text fits a fixed total duration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim", meta = (
		EditCondition = "Effect != ETextAnimEffect::None",
		EditConditionHides))
	ETextAnimTimingMode TimingMode = ETextAnimTimingMode::PerCharacter;

	/** Stagger between consecutive characters, in seconds. 0 = all at once. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim", meta = (ClampMin = "0.0",
		EditCondition = "TimingMode == ETextAnimTimingMode::PerCharacter && Effect != ETextAnimEffect::None",
		EditConditionHides))
	float Stagger = 0.05f;

	/** Total seconds for the whole text's animation to finish, when TimingMode is Fixed Duration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim", meta = (ClampMin = "0.01",
		EditCondition = "TimingMode == ETextAnimTimingMode::FixedDuration && Effect != ETextAnimEffect::None",
		EditConditionHides))
	float TotalDuration = 1.0f;

	/** Pixel amplitude for Slide Up / Wave. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim", meta = (
		EditCondition = "Effect == ETextAnimEffect::SlideUp || Effect == ETextAnimEffect::Wave",
		EditConditionHides))
	float Amplitude = 12.f;

	/** Hz for Wave. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim", meta = (ClampMin = "0.0",
		EditCondition = "Effect == ETextAnimEffect::Wave",
		EditConditionHides))
	float Frequency = 2.f;

	/** Looping effects keep running until Deactivate() is called. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim", meta = (
		EditCondition = "Effect == ETextAnimEffect::Wave",
		EditConditionHides))
	bool bLoop = true;

	/** Easing applied to time-based effects (Fade / Slide). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim", meta = (
		EditCondition = "Effect == ETextAnimEffect::FadeIn || Effect == ETextAnimEffect::SlideUp",
		EditConditionHides))
	ETextAnimEase Ease = ETextAnimEase::EaseOut;
};
