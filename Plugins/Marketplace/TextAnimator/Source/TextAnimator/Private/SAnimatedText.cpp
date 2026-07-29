// Copyright (c) 2026 Kitsana Puengsri. All Rights Reserved.
#include "SAnimatedText.h"

#include "Algo/BinarySearch.h"
#include "Fonts/FontCache.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Internationalization/BreakIterator.h"
#include "Internationalization/Text.h"
#include "Rendering/DrawElements.h"

static bool IsLoopEffect(ETextAnimEffect Effect)
{
	return Effect == ETextAnimEffect::Wave;
}

static float ApplyEase(float T, ETextAnimEase Ease)
{
	switch (Ease)
	{
	case ETextAnimEase::EaseOut:	return 1.f - FMath::Pow(1.f - T, 3.f);
	case ETextAnimEase::EaseIn:		return T * T * T;
	case ETextAnimEase::EaseInOut:	return FMath::InterpEaseInOut(0.f, 1.f, T, 3.f);
	case ETextAnimEase::ElasticOut:
		return T >= 1.f ? 1.f
			: FMath::Pow(2.f, -10.f * T) * FMath::Sin((T - 0.075f) * (2.f * PI) / 0.3f) + 1.f;
	case ETextAnimEase::BackOut:
	{
		const float C1 = 1.70158f;
		const float U = T - 1.f;
		return 1.f + (C1 + 1.f) * U * U * U + C1 * U * U;
	}
	case ETextAnimEase::BounceOut:
	{
		const float N = 7.5625f, D = 2.75f;
		if (T < 1.f / D)      { return N * T * T; }
		if (T < 2.f / D)      { const float U = T - 1.5f / D;   return N * U * U + 0.75f; }
		if (T < 2.5f / D)     { const float U = T - 2.25f / D;  return N * U * U + 0.9375f; }
		{ const float U = T - 2.625f / D; return N * U * U + 0.984375f; }
	}
	default: return T;
	}
}

void SAnimatedText::Construct(const FArguments& InArgs)
{
	Text = InArgs._Text;
	Font = InArgs._Font;
	ColorAndOpacity = InArgs._ColorAndOpacity;
	Effect = InArgs._Effect;
	Params = InArgs._Params;
	ShadowOffset = InArgs._ShadowOffset;
	ShadowColorAndOpacity = InArgs._ShadowColorAndOpacity;
	Justification = InArgs._Justification;
	TransformPolicy = InArgs._TransformPolicy;
	MinDesiredWidth = InArgs._MinDesiredWidth;
	LineHeightPercentage = InArgs._LineHeightPercentage;
	WrapTextAt = InArgs._WrapTextAt;
	bAutoWrapText = InArgs._AutoWrapText;
	SetCanTick(bAutoWrapText);
	RebuildClusters();
}

void SAnimatedText::SetText(const FText& InText)
{
	if (!Text.EqualTo(InText))
	{
		Text = InText;
		RebuildClusters();
		Invalidate(EInvalidateWidgetReason::Layout);
	}
}

void SAnimatedText::SetFont(const FSlateFontInfo& InFont)
{
	Font = InFont;
	RebuildClusters();
	Invalidate(EInvalidateWidgetReason::Layout);
}

void SAnimatedText::SetColorAndOpacity(const FLinearColor& InColor)
{
	ColorAndOpacity = InColor;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SAnimatedText::SetEffect(ETextAnimEffect InEffect)
{
	Effect = InEffect;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SAnimatedText::SetParams(const FTextAnimParams& InParams)
{
	Params = InParams;
	// Stagger/CharAnimDuration feed the cached timing table.
	RebuildClusters();
	Invalidate(EInvalidateWidgetReason::Layout);
}

void SAnimatedText::SetShadowOffset(const FVector2D& InOffset)
{
	ShadowOffset = InOffset;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SAnimatedText::SetShadowColorAndOpacity(const FLinearColor& InColor)
{
	ShadowColorAndOpacity = InColor;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SAnimatedText::SetJustification(ETextJustify::Type InJustification)
{
	Justification = InJustification;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SAnimatedText::SetTransformPolicy(ETextTransformPolicy InPolicy)
{
	if (TransformPolicy != InPolicy)
	{
		TransformPolicy = InPolicy;
		RebuildClusters();
		Invalidate(EInvalidateWidgetReason::Layout);
	}
}

void SAnimatedText::SetMinDesiredWidth(float InMinWidth)
{
	MinDesiredWidth = InMinWidth;
	Invalidate(EInvalidateWidgetReason::Layout);
}

void SAnimatedText::SetLineHeightPercentage(float InPercentage)
{
	if (LineHeightPercentage != InPercentage)
	{
		LineHeightPercentage = InPercentage;
		RebuildClusters();
		Invalidate(EInvalidateWidgetReason::Layout);
	}
}

void SAnimatedText::SetWrapTextAt(float InWrapTextAt)
{
	if (WrapTextAt != InWrapTextAt)
	{
		WrapTextAt = InWrapTextAt;
		RebuildClusters();
		Invalidate(EInvalidateWidgetReason::Layout);
	}
}

void SAnimatedText::SetAutoWrapText(bool bInAutoWrap)
{
	if (bAutoWrapText != bInAutoWrap)
	{
		bAutoWrapText = bInAutoWrap;
		SetCanTick(bAutoWrapText);
		RebuildClusters();
		Invalidate(EInvalidateWidgetReason::Layout);
	}
}

void SAnimatedText::Tick(const FGeometry& AllottedGeometry, const double /*InCurrentTime*/, const float /*InDeltaTime*/)
{
	if (bAutoWrapText)
	{
		const float W = AllottedGeometry.GetLocalSize().X;
		if (W > 0.f && !FMath::IsNearlyEqual(W, CachedAutoWrapWidth, 0.5f))
		{
			CachedAutoWrapWidth = W;
			RebuildClusters();
			Invalidate(EInvalidateWidgetReason::Layout);
		}
	}
}

void SAnimatedText::Play()
{
	ElapsedTime = 0.f;
	bPlaying = true;
	bNeverActivated = false;
	EnsureTimer();
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SAnimatedText::EnsureTimer()
{
	if (!TimerHandle.IsValid())
	{
		TimerHandle = RegisterActiveTimer(0.f,
			FWidgetActiveTimerDelegate::CreateSP(this, &SAnimatedText::TickAnim));
	}
}

void SAnimatedText::Stop()
{
	bPlaying = false;
	if (TimerHandle.IsValid())
	{
		UnRegisterActiveTimer(TimerHandle.ToSharedRef());
		TimerHandle.Reset();
	}
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SAnimatedText::SkipToEnd()
{
	if (bPlaying)
	{
		ElapsedTime = TotalDuration();
		if (!IsLooping())
		{
			bPlaying = false;
			OnFinished.ExecuteIfBound();
		}
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

EActiveTimerReturnType SAnimatedText::TickAnim(double /*InCurrentTime*/, float InDeltaTime)
{
	ElapsedTime += InDeltaTime;

	if (!IsLooping() && ElapsedTime >= TotalDuration())
	{
		bPlaying = false;
		OnFinished.ExecuteIfBound();
	}

	Invalidate(EInvalidateWidgetReason::Paint);
	if (!bPlaying)
	{
		TimerHandle.Reset();
		return EActiveTimerReturnType::Stop;
	}
	return EActiveTimerReturnType::Continue;
}

float SAnimatedText::GetWrapWidth() const
{
	if (WrapTextAt > 0.f)
	{
		return WrapTextAt;
	}
	if (bAutoWrapText && CachedAutoWrapWidth > 0.f)
	{
		return CachedAutoWrapWidth;
	}
	return 0.f;
}

void SAnimatedText::RebuildClusters()
{
	Clusters.Reset();
	ShapedClusters.Reset();
	ClusterWidths.Reset();
	PenOffsets.Reset();
	ClusterLines.Reset();
	LineWidths.Reset();
	StartTimes.Reset();
	MaxStartTime = 0.f;
	MaxClusterDuration = Params.CharAnimDuration;
	CachedSize = FVector2D::ZeroVector;

	FString Str = Text.ToString();

	switch (TransformPolicy)
	{
	case ETextTransformPolicy::ToUpper: Str = Str.ToUpper(); break;
	case ETextTransformPolicy::ToLower: Str = Str.ToLower(); break;
	default: break;
	}

	if (Str.IsEmpty() || !FSlateApplication::IsInitialized())
	{
		return;
	}

	const TSharedRef<FSlateFontMeasure> Measure =
		FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const TSharedRef<FSlateFontCache> FontCache =
		FSlateApplication::Get().GetRenderer()->GetFontCache();
	const float LineHeight = Measure->GetMaxCharacterHeight(Font) * LineHeightPercentage;
	CachedLineHeight = LineHeight;

	// Same formula the engine shaper uses; the shaper only adds spacing between
	// glyphs inside one run, so spacing between clusters is added here.
	const float ExtraSpacing = Font.LetterSpacing != 0
		? Font.LetterSpacing * (float)Font.Size / 1000.f : 0.f;

	// Layout is computed at scale 1.0 (stable pen offsets/wrapping). Drawing uses
	// a paint-scale reshape (EnsureScaledShapes) so glyph size tracks zoom/DPI.
	auto Shape = [&](const FString& InStr) -> FShapedGlyphSequencePtr
	{
		return FontCache->ShapeUnidirectionalText(InStr, Font, 1.f,
			TextBiDi::ETextDirection::LeftToRight, GetDefaultTextShapingMethod());
	};

	// Invalidate the paint-scale cache; it rebuilds lazily in OnPaint.
	ScaledCacheScale = 0.f;
	ScaledClusters.Reset();

	// Pass 1: grapheme clusters + widths + allowed line-break points.
	struct FRawCluster
	{
		FString Str;
		FShapedGlyphSequencePtr Shaped;
		float Width = 0.f;
		int32 PlainIndex = 0;
		bool bNewline = false;
		bool bBreakAfter = false;
	};
	TArray<FRawCluster> Raw;

	TSet<int32> LineBreakPoints;
	{
		TSharedRef<IBreakIterator> LineIter = FBreakIterator::CreateLineBreakIterator();
		LineIter->SetString(Str);
		for (int32 B = LineIter->MoveToNext(); B != INDEX_NONE; B = LineIter->MoveToNext())
		{
			LineBreakPoints.Add(B);
		}
	}

	{
		TSharedRef<IBreakIterator> Iter = FBreakIterator::CreateCharacterBoundaryIterator();
		Iter->SetString(Str);
		int32 Prev = 0;
		for (int32 Cur = Iter->MoveToNext(); Cur != INDEX_NONE; Cur = Iter->MoveToNext())
		{
			FRawCluster& C = Raw.AddDefaulted_GetRef();
			C.Str = Str.Mid(Prev, Cur - Prev);
			C.PlainIndex = Prev;
			C.bBreakAfter = LineBreakPoints.Contains(Cur);
			if (C.Str.Contains(TEXT("\n")))
			{
				C.bNewline = true;
			}
			else
			{
				C.Shaped = Shape(C.Str);
				C.Width = (float)C.Shaped->GetMeasuredWidth();
			}
			Prev = Cur;
		}
	}

	// Pass 2: greedy word wrap. A "word" is a run of clusters ending at an
	// allowed break point (the line-break iterator handles Thai word breaks).
	const float WrapW = GetWrapWidth();
	float PenX = 0.f;
	float PenY = 0.f;
	int32 Line = 0;

	auto EndLine = [&]()
	{
		LineWidths.Add(FMath::Max(0.f, PenX - ExtraSpacing));
		PenX = 0.f;
		PenY += LineHeight;
		++Line;
	};

	auto PlaceCluster = [&](const FRawCluster& C)
	{
		Clusters.Add(C.Str);
		ShapedClusters.Add(C.Shaped);
		ClusterWidths.Add(C.Width);
		PenOffsets.Add(FVector2f(PenX, PenY));
		ClusterLines.Add(Line);
		CachedSize.X = FMath::Max(CachedSize.X, (double)(PenX + C.Width));
		PenX += C.Width + ExtraSpacing;
	};

	int32 i = 0;
	while (i < Raw.Num())
	{
		if (Raw[i].bNewline)
		{
			EndLine();
			++i;
			continue;
		}

		// Collect one word: clusters up to and including the next break point.
		int32 WordEnd = i;
		float WordWidth = 0.f;
		while (WordEnd < Raw.Num() && !Raw[WordEnd].bNewline)
		{
			WordWidth += Raw[WordEnd].Width + ExtraSpacing;
			if (Raw[WordEnd].bBreakAfter)
			{
				break;
			}
			++WordEnd;
		}
		WordEnd = FMath::Min(WordEnd, Raw.Num() - 1);

		if (WrapW > 0.f && PenX > 0.f && PenX + WordWidth - ExtraSpacing > WrapW)
		{
			EndLine();
		}

		for (int32 c = i; c <= WordEnd; ++c)
		{
			if (Raw[c].bNewline)
			{
				break;
			}
			// Hard-break inside an oversized word.
			if (WrapW > 0.f && PenX > 0.f && PenX + Raw[c].Width > WrapW)
			{
				EndLine();
			}
			PlaceCluster(Raw[c]);
		}
		i = WordEnd + 1;
	}
	LineWidths.Add(FMath::Max(0.f, PenX - ExtraSpacing));
	CachedSize.Y = PenY + LineHeight;

	// Timing table: staggered start time per cluster.
	const int32 N = Clusters.Num();
	StartTimes.SetNum(N);
	float EffectiveStagger = Params.Stagger;
	if (Params.TimingMode == ETextAnimTimingMode::FixedDuration)
	{
		EffectiveStagger = (N > 1)
			? (Params.TotalDuration - Params.CharAnimDuration) / (N - 1)
			: 0.f;
		EffectiveStagger = FMath::Max(EffectiveStagger, 0.f);
	}
	float t = 0.f;
	for (int32 c = 0; c < N; ++c)
	{
		StartTimes[c] = t;
		MaxStartTime = t;
		MaxClusterDuration = FMath::Max(MaxClusterDuration, Params.CharAnimDuration);
		t += EffectiveStagger;
	}
}

void SAnimatedText::EnsureScaledShapes(float FontScale) const
{
	if (FMath::IsNearlyEqual(FontScale, ScaledCacheScale))
	{
		return; // already built at this scale
	}
	ScaledCacheScale = FontScale;
	ScaledClusters.Reset();

	if (!FSlateApplication::IsInitialized())
	{
		return;
	}

	const TSharedRef<FSlateFontCache> FontCache =
		FSlateApplication::Get().GetRenderer()->GetFontCache();
	auto ShapeScaled = [&](const FString& InStr) -> FShapedGlyphSequencePtr
	{
		return FontCache->ShapeUnidirectionalText(InStr, Font, FontScale,
			TextBiDi::ETextDirection::LeftToRight, GetDefaultTextShapingMethod());
	};

	ScaledClusters.Reserve(Clusters.Num());
	for (int32 c = 0; c < Clusters.Num(); ++c)
	{
		// Newline clusters have no glyph; keep the slot null to stay index-aligned.
		ScaledClusters.Add(Clusters[c].Contains(TEXT("\n")) ? nullptr : ShapeScaled(Clusters[c]));
	}
}

FVector2D SAnimatedText::ComputeDesiredSize(float /*LayoutScaleMultiplier*/) const
{
	return FVector2D(FMath::Max(CachedSize.X, (double)MinDesiredWidth), CachedSize.Y);
}

float SAnimatedText::TotalDuration() const
{
	return MaxStartTime + MaxClusterDuration;
}

bool SAnimatedText::IsLooping() const
{
	return Params.bLoop && IsLoopEffect(Effect);
}

float SAnimatedText::GetStartTime(int32 Index) const
{
	return StartTimes.IsValidIndex(Index) ? StartTimes[Index] : Index * Params.Stagger;
}

void SAnimatedText::ComputeClusterAnim(int32 Index,
	float& OutAlpha, FVector2f& OutOffset) const
{
	OutAlpha = 1.f;
	OutOffset = FVector2f::ZeroVector;

	const float Start = GetStartTime(Index);
	const float T = FMath::Clamp(
		(ElapsedTime - Start) / FMath::Max(Params.CharAnimDuration, KINDA_SMALL_NUMBER), 0.f, 1.f);
	const float Eased = ApplyEase(T, Params.Ease);

	switch (Effect)
	{
	case ETextAnimEffect::Typewriter:
		OutAlpha = (ElapsedTime >= Start) ? 1.f : 0.f;
		break;

	case ETextAnimEffect::FadeIn:
		OutAlpha = Eased;
		break;

	case ETextAnimEffect::SlideUp:
		OutAlpha = T;
		OutOffset.Y = (1.f - Eased) * Params.Amplitude;
		break;

	case ETextAnimEffect::Wave:
		OutOffset.Y = FMath::Sin(2.f * PI * Params.Frequency * ElapsedTime - Index * 0.6f)
			* Params.Amplitude;
		break;

	default:
		break;
	}
}

int32 SAnimatedText::OnPaint(const FPaintArgs& /*Args*/, const FGeometry& AllottedGeometry,
	const FSlateRect& /*MyCullingRect*/, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	// Reshape glyphs at the actual paint scale so their size tracks UMG zoom /
	// high-DPI (MakeShapedText draws at the shaped pixel size). Geometry/positions
	// below are unchanged (still 1.0 layout). Quantize lightly to bound cache churn.
	const float RawScale = AllottedGeometry.GetAccumulatedLayoutTransform().GetScale();
	const float FontScale = FMath::Clamp(FMath::RoundToFloat(RawScale * 20.f) / 20.f, 0.25f, 8.f);
	EnsureScaledShapes(FontScale);

	const FLinearColor BaseColor = ColorAndOpacity * InWidgetStyle.GetColorAndOpacityTint();
	const ESlateDrawEffect DrawEffects =
		(bParentEnabled && IsEnabled()) ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

	const float LocalWidth = AllottedGeometry.GetLocalSize().X;
	auto LineStartX = [&](int32 Line) -> float
	{
		switch (Justification)
		{
		case ETextJustify::Center: return FMath::Max(0.f, (LocalWidth - LineWidths[Line]) * 0.5f);
		case ETextJustify::Right:  return FMath::Max(0.f, LocalWidth - LineWidths[Line]);
		default:                   return 0.f;
		}
	};

	const bool bHasShadow = ShadowColorAndOpacity.A > 0.f && !ShadowOffset.IsNearlyZero();
	const FVector2f ShadowOffset2f((float)ShadowOffset.X, (float)ShadowOffset.Y);
	const float TintA = InWidgetStyle.GetColorAndOpacityTint().A;

	auto DrawSeq = [&](const FShapedGlyphSequencePtr& Seq, const FVector2f& At,
		const FLinearColor& Tint, const FLinearColor& Outline, int32 Layer)
	{
		if (!Seq.IsValid())
		{
			return;
		}
		FPaintGeometry Geo = AllottedGeometry.ToPaintGeometry(
			AllottedGeometry.GetLocalSize(), FSlateLayoutTransform(At));
		FSlateDrawElement::MakeShapedText(OutDrawElements, Layer, Geo,
			Seq.ToSharedRef(), DrawEffects, Tint, Outline);
	};

	for (int32 i = 0; i < Clusters.Num(); ++i)
	{
		float Alpha = 1.f;
		FVector2f AnimOffset = FVector2f::ZeroVector;

		if (bPlaying || bNeverActivated)
		{
			// bNeverActivated + ElapsedTime==0 renders the effect's natural
			// starting pose (usually hidden) instead of the Stop() fallback below.
			ComputeClusterAnim(i, Alpha, AnimOffset);
		}
		if (Alpha <= 0.f)
		{
			continue;
		}

		const FShapedGlyphSequencePtr Shaped =
			(ScaledClusters.IsValidIndex(i) && ScaledClusters[i].IsValid())
				? ScaledClusters[i] : ShapedClusters[i];

		const FVector2f Pos = PenOffsets[i] + AnimOffset
			+ FVector2f(LineStartX(ClusterLines[i]), 0.f);

		if (bHasShadow)
		{
			FLinearColor Shadow = ShadowColorAndOpacity;
			Shadow.A *= Alpha * TintA;
			DrawSeq(Shaped, Pos + ShadowOffset2f, Shadow, Shadow, LayerId);
		}

		FLinearColor Color = BaseColor;
		Color.A = BaseColor.A * Alpha;
		FLinearColor OutlineColor =
			Font.OutlineSettings.OutlineColor * InWidgetStyle.GetColorAndOpacityTint();
		OutlineColor.A *= Alpha;
		DrawSeq(Shaped, Pos, Color, OutlineColor, LayerId + 1);
	}

	return LayerId + 1;
}
