// Copyright (c) 2026 Kitsana Puengsri. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Fonts/ShapedTextFwd.h"
#include "Fonts/SlateFontInfo.h"
#include "Framework/Text/TextLayout.h"
#include "Widgets/SLeafWidget.h"
#include "TextAnimTypes.h"

/**
 * Leaf widget that paints text one grapheme cluster at a time so every
 * cluster can get its own offset/opacity/color. Clusters (not chars)
 * keep Thai combining vowels/tone marks attached to their base consonant.
 */
class TEXTANIMATOR_API SAnimatedText : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SAnimatedText)
		: _ColorAndOpacity(FLinearColor::White)
		, _Effect(ETextAnimEffect::None)
		, _ShadowOffset(1.f, 1.f)
		, _ShadowColorAndOpacity(0.f, 0.f, 0.f, 0.f)
		, _Justification(ETextJustify::Left)
		, _TransformPolicy(ETextTransformPolicy::None)
		, _MinDesiredWidth(0.f)
		, _LineHeightPercentage(1.f)
		, _WrapTextAt(0.f)
		, _AutoWrapText(false)
	{}
		SLATE_ARGUMENT(FText, Text)
		SLATE_ARGUMENT(FSlateFontInfo, Font)
		SLATE_ARGUMENT(FLinearColor, ColorAndOpacity)
		SLATE_ARGUMENT(ETextAnimEffect, Effect)
		SLATE_ARGUMENT(FTextAnimParams, Params)
		SLATE_ARGUMENT(FVector2D, ShadowOffset)
		SLATE_ARGUMENT(FLinearColor, ShadowColorAndOpacity)
		SLATE_ARGUMENT(ETextJustify::Type, Justification)
		SLATE_ARGUMENT(ETextTransformPolicy, TransformPolicy)
		SLATE_ARGUMENT(float, MinDesiredWidth)
		SLATE_ARGUMENT(float, LineHeightPercentage)
		SLATE_ARGUMENT(float, WrapTextAt)
		SLATE_ARGUMENT(bool, AutoWrapText)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetText(const FText& InText);
	void SetFont(const FSlateFontInfo& InFont);
	void SetColorAndOpacity(const FLinearColor& InColor);
	void SetEffect(ETextAnimEffect InEffect);
	void SetParams(const FTextAnimParams& InParams);
	void SetShadowOffset(const FVector2D& InOffset);
	void SetShadowColorAndOpacity(const FLinearColor& InColor);
	void SetJustification(ETextJustify::Type InJustification);
	void SetTransformPolicy(ETextTransformPolicy InPolicy);
	void SetMinDesiredWidth(float InMinWidth);
	void SetLineHeightPercentage(float InPercentage);
	void SetWrapTextAt(float InWrapTextAt);
	void SetAutoWrapText(bool bInAutoWrap);

	/** Restart the entrance animation from the beginning. */
	void Play();
	/** Stop and show the final (fully visible) text. */
	void Stop();
	/** Jump to the fully-revealed end state instantly. */
	void SkipToEnd();
	bool IsPlaying() const { return bPlaying; }

	/** Fired when the entrance animation finishes naturally. */
	FSimpleDelegate OnFinished;

	// SWidget
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	EActiveTimerReturnType TickAnim(double InCurrentTime, float InDeltaTime);
	void EnsureTimer();
	void RebuildClusters();
	/** Reshape the 1.0 caches at the given paint scale so glyphs track zoom/DPI crisply. */
	void EnsureScaledShapes(float FontScale) const;
	float GetStartTime(int32 Index) const;
	void ComputeClusterAnim(int32 Index,
		float& OutAlpha, FVector2f& OutOffset) const;
	float GetWrapWidth() const;
	float TotalDuration() const;
	bool IsLooping() const;

	FText Text;
	FSlateFontInfo Font;
	FLinearColor ColorAndOpacity = FLinearColor::White;
	ETextAnimEffect Effect = ETextAnimEffect::None;
	FTextAnimParams Params;

	FVector2D ShadowOffset = FVector2D(1.f, 1.f);
	FLinearColor ShadowColorAndOpacity = FLinearColor(0.f, 0.f, 0.f, 0.f);
	ETextJustify::Type Justification = ETextJustify::Left;
	ETextTransformPolicy TransformPolicy = ETextTransformPolicy::None;
	float MinDesiredWidth = 0.f;
	float LineHeightPercentage = 1.f;
	float WrapTextAt = 0.f;
	bool bAutoWrapText = false;
	float CachedAutoWrapWidth = 0.f;

	TArray<FString> Clusters;
	TArray<FShapedGlyphSequencePtr> ShapedClusters;	// pre-shaped, honors LetterSpacing/Skew

	// Paint-scale shaped caches: same source strings reshaped at the actual paint
	// scale. MakeShapedText draws glyphs at their shaped pixel size (NOT multiplied
	// by the paint geometry scale), so to make text track UMG zoom / high-DPI we
	// shape at the paint scale. Geometry/positions still come from the 1.0 layout.
	// Mutable: (re)built lazily from const OnPaint.
	mutable float ScaledCacheScale = 0.f;
	mutable TArray<FShapedGlyphSequencePtr> ScaledClusters;

	TArray<float> ClusterWidths;
	TArray<FVector2f> PenOffsets;
	TArray<int32> ClusterLines;	// line index per cluster, for justification
	TArray<float> LineWidths;
	TArray<float> StartTimes;		// entrance start time per cluster
	float MaxStartTime = 0.f;
	float MaxClusterDuration = 0.f;
	FVector2D CachedSize = FVector2D::ZeroVector;
	float CachedLineHeight = 0.f;

	float ElapsedTime = 0.f;
	bool bPlaying = false;
	// True until the first Play(): renders the effect's own T=0 pose (usually
	// hidden) instead of the Stop()/Deactivate() fallback of full text, so
	// entrance effects don't flash fully-visible before Activate() is called.
	bool bNeverActivated = true;
	TSharedPtr<FActiveTimerHandle> TimerHandle;
};
