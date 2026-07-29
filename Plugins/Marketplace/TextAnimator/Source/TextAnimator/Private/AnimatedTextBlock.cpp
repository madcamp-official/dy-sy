// Copyright (c) 2026 Kitsana Puengsri. All Rights Reserved.
#include "AnimatedTextBlock.h"

#include "Engine/Font.h"
#include "SAnimatedText.h"
#include "TextAnimPreset.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "TextAnimator"

UAnimatedTextBlock::UAnimatedTextBlock()
{
	if (!IsRunningDedicatedServer())
	{
		static ConstructorHelpers::FObjectFinder<UFont> RobotoFontObj(TEXT("/Engine/EngineFonts/Roboto"));
		Font = FSlateFontInfo(RobotoFontObj.Object, 24, FName("Bold"));
	}
	Text = LOCTEXT("DefaultText", "Animated Text");
	Params.Effect = Effect;
}

ETextAnimEffect UAnimatedTextBlock::GetEffectiveEffect() const
{
	return Preset ? Preset->Effect : Effect;
}

const FTextAnimParams& UAnimatedTextBlock::GetEffectiveParams() const
{
	return Preset ? Preset->Params : Params;
}

TSharedRef<SWidget> UAnimatedTextBlock::RebuildWidget()
{
	MyAnimatedText = SNew(SAnimatedText)
		.Text(Text)
		.Font(Font)
		.ColorAndOpacity(ColorAndOpacity)
		.Effect(GetEffectiveEffect())
		.Params(GetEffectiveParams())
		.ShadowOffset(ShadowOffset)
		.ShadowColorAndOpacity(ShadowColorAndOpacity)
		.Justification(Justification)
		.TransformPolicy(TextTransformPolicy)
		.MinDesiredWidth(MinDesiredWidth)
		.LineHeightPercentage(LineHeightPercentage)
		.WrapTextAt(WrapTextAt)
		.AutoWrapText(bAutoWrapText);

	MyAnimatedText->OnFinished = FSimpleDelegate::CreateWeakLambda(this,
		[this]() { OnAnimationFinished.Broadcast(); });

	return MyAnimatedText.ToSharedRef();
}

void UAnimatedTextBlock::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	if (MyAnimatedText.IsValid())
	{
		MyAnimatedText->SetText(Text);
		MyAnimatedText->SetFont(Font);
		MyAnimatedText->SetColorAndOpacity(ColorAndOpacity);
		MyAnimatedText->SetEffect(GetEffectiveEffect());
		MyAnimatedText->SetParams(GetEffectiveParams());
		MyAnimatedText->SetShadowOffset(ShadowOffset);
		MyAnimatedText->SetShadowColorAndOpacity(ShadowColorAndOpacity);
		MyAnimatedText->SetJustification(Justification);
		MyAnimatedText->SetTransformPolicy(TextTransformPolicy);
		MyAnimatedText->SetMinDesiredWidth(MinDesiredWidth);
		MyAnimatedText->SetLineHeightPercentage(LineHeightPercentage);
		MyAnimatedText->SetWrapTextAt(WrapTextAt);
		MyAnimatedText->SetAutoWrapText(bAutoWrapText);

		if (IsDesignTime() ? bPreviewInDesigner : bAutoActivate)
		{
			Activate();
		}
	}
}

void UAnimatedTextBlock::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyAnimatedText.Reset();
}

#if WITH_EDITOR
const FText UAnimatedTextBlock::GetPaletteCategory()
{
	return LOCTEXT("PaletteCategory", "Text Animator");
}

void UAnimatedTextBlock::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// Keep the hidden mirror in sync so per-effect EditConditions evaluate correctly.
	Params.Effect = Effect;
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UAnimatedTextBlock::Activate()
{
	if (MyAnimatedText.IsValid())
	{
		MyAnimatedText->Play();
	}
}

void UAnimatedTextBlock::Deactivate()
{
	if (MyAnimatedText.IsValid())
	{
		MyAnimatedText->Stop();
	}
}

void UAnimatedTextBlock::SkipToEnd()
{
	if (MyAnimatedText.IsValid())
	{
		MyAnimatedText->SkipToEnd();
	}
}

bool UAnimatedTextBlock::IsActive() const
{
	return MyAnimatedText.IsValid() && MyAnimatedText->IsPlaying();
}

void UAnimatedTextBlock::SetText(FText InText, bool bReplay)
{
	Text = InText;
	if (MyAnimatedText.IsValid())
	{
		MyAnimatedText->SetText(InText);
		if (bReplay)
		{
			MyAnimatedText->Play();
		}
	}
}

void UAnimatedTextBlock::SetEffect(ETextAnimEffect InEffect, bool bReplay)
{
	Effect = InEffect;
	Params.Effect = InEffect;
	if (MyAnimatedText.IsValid())
	{
		MyAnimatedText->SetEffect(GetEffectiveEffect());
		if (bReplay)
		{
			MyAnimatedText->Play();
		}
	}
}

void UAnimatedTextBlock::SetParams(FTextAnimParams InParams)
{
	InParams.Effect = Effect;
	Params = InParams;
	if (MyAnimatedText.IsValid())
	{
		MyAnimatedText->SetParams(GetEffectiveParams());
	}
}

void UAnimatedTextBlock::SetPreset(UTextAnimPreset* InPreset, bool bReplay)
{
	Preset = InPreset;
	if (MyAnimatedText.IsValid())
	{
		MyAnimatedText->SetEffect(GetEffectiveEffect());
		MyAnimatedText->SetParams(GetEffectiveParams());
		if (bReplay)
		{
			MyAnimatedText->Play();
		}
	}
}

void UAnimatedTextBlock::SetColorAndOpacity(FLinearColor InColor)
{
	ColorAndOpacity = InColor;
	if (MyAnimatedText.IsValid())
	{
		MyAnimatedText->SetColorAndOpacity(InColor);
	}
}

void UAnimatedTextBlock::SetShadowColorAndOpacity(FLinearColor InColor)
{
	ShadowColorAndOpacity = InColor;
	if (MyAnimatedText.IsValid())
	{
		MyAnimatedText->SetShadowColorAndOpacity(InColor);
	}
}

void UAnimatedTextBlock::SetJustification(TEnumAsByte<ETextJustify::Type> InJustification)
{
	Justification = InJustification;
	if (MyAnimatedText.IsValid())
	{
		MyAnimatedText->SetJustification(InJustification);
	}
}

#undef LOCTEXT_NAMESPACE
