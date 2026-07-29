// Copyright (c) 2026 Kitsana Puengsri. All Rights Reserved.
// Text Animator editor module: "Text Animator Presets" showroom tab.
// Open from Window menu; drag a preset card into the UMG Designer to spawn a
// configured Animated Text Block.

#include "Modules/ModuleManager.h"

#include "Animation/CurveSequence.h"
#include "AnimatedTextBlock.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "DragDrop/WidgetTemplateDragDropOp.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Framework/Application/MenuStack.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Layout/WidgetPath.h"
#include "HAL/IConsoleManager.h"
#include "Interfaces/IMainFrameModule.h"
#include "Interfaces/IPluginManager.h"
#include "Engine/Font.h"
#include "Misc/Paths.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "Brushes/SlateImageBrush.h"
#include "Styling/ISlateStyle.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Textures/SlateIcon.h"
#include "Misc/ConfigCacheIni.h"
#include "PropertyCustomizationHelpers.h"
#include "SAnimatedText.h"
#include "Styling/CoreStyle.h"
#include "TextAnimPreset.h"
#include "TextAnimTypes.h"
#include "WidgetTemplate.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SToolTip.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#define LOCTEXT_NAMESPACE "TextAnimatorEditor"

static const FName TextAnimShowroomTabId(TEXT("TextAnimPresetShowroom"));

// ---------------------------------------------------------------------------
// Test window: run `TextAnimator.Test` in the console to preview every effect.
static void OpenTextAnimatorTestWindow()
{
	if (!FSlateApplication::IsInitialized())
	{
		return;
	}

	const FSlateFontInfo BigFont = FCoreStyle::GetDefaultFontStyle("Regular", 26);
	const FSlateFontInfo LabelFont = FCoreStyle::GetDefaultFontStyle("Bold", 11);
	const FSlateFontInfo NoteFont = FCoreStyle::GetDefaultFontStyle("Regular", 9);
	const FText SampleText = FText::FromString(TEXT("Hello Unreal World!"));

	TSharedRef<FTextAnimParams> Params = MakeShared<FTextAnimParams>();
	TSharedRef<TArray<TSharedPtr<SAnimatedText>>> AnimWidgets =
		MakeShared<TArray<TSharedPtr<SAnimatedText>>>();

	auto ApplyParams = [AnimWidgets, Params]()
	{
		for (const TSharedPtr<SAnimatedText>& W : *AnimWidgets)
		{
			if (W.IsValid())
			{
				W->SetParams(*Params);
			}
		}
	};

	auto PlayAll = [AnimWidgets, ApplyParams]()
	{
		ApplyParams();
		for (const TSharedPtr<SAnimatedText>& W : *AnimWidgets)
		{
			if (W.IsValid())
			{
				W->Play();
			}
		}
	};

	// Hover tooltip: effect name + a short note.
	auto MakeTip = [&](const FString& Title, const FString& Note)
	{
		return SNew(SToolTip)
		[
			SNew(SBox).MaxDesiredWidth(420.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(2.f)
				[
					SNew(STextBlock)
						.Text(FText::FromString(Title))
						.Font(LabelFont)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(2.f)
				[
					SNew(STextBlock)
						.Text(FText::FromString(Note))
						.Font(NoteFont)
						.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f))
						.AutoWrapText(true)
				]
			]
		];
	};

	// --- Effect rows ---
	TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
	auto AddRow = [&](const FString& Label, ETextAnimEffect Effect, const FString& Note)
	{
		TSharedPtr<SAnimatedText> Anim;
		TSharedRef<SWidget> Row =
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 2.f)
			[
				SNew(STextBlock)
					.Text(FText::FromString(Label))
					.Font(LabelFont)
					.ColorAndOpacity(FLinearColor(0.5f, 0.7f, 1.f))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SAssignNew(Anim, SAnimatedText)
					.Text(SampleText)
					.Font(BigFont)
					.Effect(Effect)
					.Params(*Params)
					.ShadowOffset(FVector2D(2.f, 2.f))
					.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.6f))
			];
		Row->SetToolTip(MakeTip(Label, Note));
		List->AddSlot().AutoHeight().Padding(12.f, 10.f)[Row];
		AnimWidgets->Add(Anim);
	};

	AddRow(TEXT("Typewriter"), ETextAnimEffect::Typewriter,
		TEXT("Char Delay = seconds between characters."));
	AddRow(TEXT("Fade In"), ETextAnimEffect::FadeIn,
		TEXT("Duration = fade time per character. Ease comes from the widget's Ease setting."));
	AddRow(TEXT("Slide Up"), ETextAnimEffect::SlideUp,
		TEXT("Amplitude = slide distance in px."));
	AddRow(TEXT("Wave"), ETextAnimEffect::Wave,
		TEXT("Amplitude = wave height px, Frequency = Hz. Loops while playing (Loop param)."));

	// --- Parameter box ---
	TSharedRef<SHorizontalBox> ParamBox = SNew(SHorizontalBox);
	auto AddParam = [&](const FString& Name, float MinV, float MaxV, float Delta,
		TFunction<float()> Get, TFunction<void(float)> Set)
	{
		ParamBox->AddSlot().AutoWidth().Padding(6.f, 0.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(FText::FromString(Name)).Font(LabelFont)
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBox).WidthOverride(100.f)
				[
					SNew(SSpinBox<float>)
						.MinValue(MinV)
						.MaxValue(MaxV)
						.Delta(Delta)
						.Value_Lambda([Get]() { return Get(); })
						.OnValueChanged_Lambda([Set, ApplyParams](float V)
						{
							Set(V);
							ApplyParams();
						})
				]
			]
		];
	};

	AddParam(TEXT("Duration"), 0.01f, 5.f, 0.01f,
		[Params]() { return Params->CharAnimDuration; },
		[Params](float V) { Params->CharAnimDuration = V; });
	AddParam(TEXT("Char Delay"), 0.f, 1.f, 0.005f,
		[Params]() { return Params->Stagger; },
		[Params](float V) { Params->Stagger = V; });
	AddParam(TEXT("Amplitude"), 0.f, 100.f, 0.5f,
		[Params]() { return Params->Amplitude; },
		[Params](float V) { Params->Amplitude = V; });
	AddParam(TEXT("Frequency"), 0.f, 20.f, 0.1f,
		[Params]() { return Params->Frequency; },
		[Params](float V) { Params->Frequency = V; });

	ParamBox->AddSlot().AutoWidth().Padding(10.f, 0.f, 6.f, 0.f).VAlign(VAlign_Bottom)
	[
		SNew(SCheckBox)
			.IsChecked_Lambda([Params]()
			{
				return Params->bLoop ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
			.OnCheckStateChanged_Lambda([Params, ApplyParams](ECheckBoxState State)
			{
				Params->bLoop = (State == ECheckBoxState::Checked);
				ApplyParams();
			})
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Loop")))
			]
	];

	ParamBox->AddSlot().AutoWidth().Padding(14.f, 0.f, 0.f, 0.f).VAlign(VAlign_Bottom)
	[
		SNew(SButton)
			.OnClicked_Lambda([PlayAll]() { PlayAll(); return FReply::Handled(); })
			[
				SNew(STextBlock)
					.Text(NSLOCTEXT("TextAnimator", "Replay", "Replay"))
			]
	];

	// Optional plugin icon (Resources/Icon128.png). Static so the brush outlives the window.
	static TSharedPtr<FSlateDynamicImageBrush> IconBrush;
	if (!IconBrush.IsValid())
	{
		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("TextAnimator"));
		if (Plugin.IsValid())
		{
			const FString IconPath = Plugin->GetBaseDir() / TEXT("Resources/Icon128.png");
			if (FPaths::FileExists(IconPath))
			{
				IconBrush = MakeShareable(new FSlateDynamicImageBrush(FName(*IconPath), FVector2D(72.f, 72.f)));
			}
		}
	}

	// --- Left column: icon + written how-to + features + startup checkbox ---
	TSharedRef<SVerticalBox> LeftCol = SNew(SVerticalBox);
	if (IconBrush.IsValid())
	{
		LeftCol->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 4.f, 0.f, 10.f)
		[
			SNew(SBox).WidthOverride(72.f).HeightOverride(72.f)
			[
				SNew(SImage).Image(IconBrush.Get())
			]
		];
	}
	LeftCol->AddSlot().AutoHeight().Padding(2.f)
	[
		SNew(STextBlock)
			.Text(NSLOCTEXT("TextAnimator", "WelcomeTitle", "Ez Text Animation — Getting Started"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
			.AutoWrapText(true)
	];
	LeftCol->AddSlot().AutoHeight().Padding(2.f, 8.f, 2.f, 2.f)
	[
		SNew(STextBlock).Text(NSLOCTEXT("TextAnimator", "WelcomeFeaturesLabel", "Features"))
			.Font(LabelFont).ColorAndOpacity(FLinearColor(0.5f, 0.7f, 1.f))
	];
	LeftCol->AddSlot().AutoHeight().Padding(2.f)
	[
		SNew(STextBlock)
			.Text(NSLOCTEXT("TextAnimator", "WelcomeFeatures",
				"4 per-character effects: Typewriter, Fade In, Slide Up, and Wave. Easing, word-wrap (Thai-aware), drop shadow, justification, and drag-and-drop presets."))
			.Font(NoteFont).ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f)).AutoWrapText(true)
	];
	LeftCol->AddSlot().AutoHeight().Padding(2.f, 8.f, 2.f, 2.f)
	[
		SNew(STextBlock).Text(NSLOCTEXT("TextAnimator", "BasicUseLabel", "Basic use"))
			.Font(LabelFont).ColorAndOpacity(FLinearColor(0.5f, 0.7f, 1.f))
	];
	LeftCol->AddSlot().AutoHeight().Padding(2.f)
	[
		SNew(STextBlock)
			.Text(NSLOCTEXT("TextAnimator", "BasicUse",
				"1. In a Widget Blueprint, open the Palette, search 'Animated Text Block', and drag it onto the canvas.\n\n"
				"2. Select it. In Details, type your Text and choose an Effect. Adjust the Params shown for that effect.\n\n"
				"3. It plays automatically when the widget appears. Turn on 'Preview In Designer' to watch it while you edit.\n\n"
				"4. From Blueprints: Activate replays it, SetText changes the text, SkipToEnd finishes instantly, and the On Animation Finished event tells you when it's done.\n\n"
				"5. Reuse a look: drag a preset card from this window into the Designer, or assign a Preset asset to the widget's Preset slot.\n\n"
				"6. Faster way: open Window > Text Animator Presets to launch the Preset Showroom, then drag any preset card straight into your Widget Blueprint's Designer canvas."))
			.Font(NoteFont).ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f)).AutoWrapText(true)
	];
	LeftCol->AddSlot().AutoHeight().Padding(2.f, 12.f, 2.f, 2.f)
	[
		SNew(SCheckBox)
			.IsChecked_Lambda([]()
			{
				bool bShow = true;
				GConfig->GetBool(TEXT("EasyTextAnimation"), TEXT("ShowWelcomeOnStartup"), bShow, GEditorPerProjectIni);
				return bShow ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
			.OnCheckStateChanged_Lambda([](ECheckBoxState State)
			{
				const bool bValue = (State == ECheckBoxState::Checked);
				GConfig->SetBool(TEXT("EasyTextAnimation"), TEXT("ShowWelcomeOnStartup"), bValue, GEditorPerProjectIni);
				GConfig->Flush(false, GEditorPerProjectIni);
			})
			[
				SNew(STextBlock)
					.Text(NSLOCTEXT("TextAnimator", "ShowOnStartup", "Show this window when the editor starts"))
					.AutoWrapText(true)
			]
	];

	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(NSLOCTEXT("TextAnimator", "TestWindowTitle", "Ez Text Animation"))
		.ClientSize(FVector2D(1040.f, 720.f))
		.SupportsMaximize(false)
		[
			SNew(SHorizontalBox)
			// LEFT: instructions
			+ SHorizontalBox::Slot().AutoWidth().Padding(8.f)
			[
				SNew(SBox).WidthOverride(360.f)
				[
					SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
						.BorderBackgroundColor(FLinearColor(0.03f, 0.03f, 0.05f))
						.Padding(12.f)
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							LeftCol
						]
					]
				]
			]
			// RIGHT: parameter box + gallery of all animations
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(8.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(4.f, 12.f, 4.f, 4.f)
				[
					ParamBox
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(4.f, 0.f, 4.f, 6.f)
				[
					SNew(STextBlock)
						.Text(NSLOCTEXT("TextAnimator", "HoverHint", "Hover a row to see what each effect does"))
						.Font(NoteFont)
						.ColorAndOpacity(FLinearColor(0.55f, 0.55f, 0.55f))
				]
				+ SVerticalBox::Slot().FillHeight(1.f)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						List
					]
				]
			]
		];

	// Keep the window usable: don't let the user shrink it below its design size.
	Window->SetSizeLimits(FWindowSizeLimits()
		.SetMinWidth(1040.f)
		.SetMinHeight(720.f));

	FSlateApplication::Get().AddWindow(Window);
	PlayAll();
}

static FAutoConsoleCommand GTextAnimatorTestCmd(
	TEXT("TextAnimator.Test"),
	TEXT("Open a test window showing every Text Animator effect."),
	FConsoleCommandDelegate::CreateStatic(&OpenTextAnimatorTestWindow));

// ---------------------------------------------------------------------------
// Widget template dropped into the UMG Designer

class FTextAnimPresetTemplate : public FWidgetTemplate
{
public:
	ETextAnimEffect Effect = ETextAnimEffect::Typewriter;
	FTextAnimParams Params;
	TWeakObjectPtr<UTextAnimPreset> Asset;

	virtual FText GetCategory() const override
	{
		return LOCTEXT("TemplateCategory", "Text Animator");
	}

	virtual UWidget* Create(UWidgetTree* Tree) override
	{
		UAnimatedTextBlock* Widget = Tree->ConstructWidget<UAnimatedTextBlock>(
			UAnimatedTextBlock::StaticClass());
		if (Asset.IsValid())
		{
			Widget->Preset = Asset.Get();
		}
		else
		{
			Widget->Effect = Effect;
			Widget->Params = Params;
			Widget->Params.Effect = Effect;
		}
		return Widget;
	}

	virtual TSharedRef<IToolTip> GetToolTip() const override
	{
		return SNew(SToolTip).Text(Name);
	}
};

// ---------------------------------------------------------------------------
// Draggable preset card

DECLARE_DELEGATE(FOnPresetCardClicked);

class SPresetCard : public SBorder
{
public:
	SLATE_BEGIN_ARGS(SPresetCard) {}
		SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_EVENT(FOnPresetCardClicked, OnClicked)
	SLATE_END_ARGS()

	TFunction<TSharedRef<FWidgetTemplate>()> MakeTemplate;

	void Construct(const FArguments& InArgs)
	{
		OnClicked = InArgs._OnClicked;
		SBorder::Construct(SBorder::FArguments()
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor(0.05f, 0.05f, 0.07f))
			.Padding(10.f)
			[
				InArgs._Content.Widget
			]);
	}

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			OnClicked.ExecuteIfBound();
			return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
		}
		return FReply::Unhandled();
	}

	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (MakeTemplate)
		{
			return FReply::Handled().BeginDragDrop(
				FWidgetTemplateDragDropOp::New(MakeTemplate()));
		}
		return FReply::Unhandled();
	}

private:
	FOnPresetCardClicked OnClicked;
};

// ---------------------------------------------------------------------------
// Showroom tab content

class STextAnimPresetShowroom : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STextAnimPresetShowroom) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		WorkingEffect = MakeShared<ETextAnimEffect>(ETextAnimEffect::Typewriter);
		WorkingParams = MakeShared<FTextAnimParams>();

		PreviewFontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 24);

		// Slow looping pulse (~1.2s cycle) that makes the help (i) button glow.
		HelpPulseCurve = FCurveSequence(0.f, 1.2f, ECurveEaseFunction::Linear);
		HelpPulseCurve.Play(this->AsShared(), /*bPlayLooped*/ true);

		const UEnum* EffectEnum = StaticEnum<ETextAnimEffect>();
		// Skip index 0 (None) and the trailing _MAX: only the 4 real effects.
		for (int32 i = 1; i < EffectEnum->NumEnums() - 1; ++i)
		{
			EffectOptions.Add(MakeShared<int32>(i));
		}

		const FSlateFontInfo LabelFont = FCoreStyle::GetDefaultFontStyle("Bold", 10);
		const FSlateFontInfo NoteFont = FCoreStyle::GetDefaultFontStyle("Regular", 9);

		// --- Working-preset editor (top section) ---
		TSharedRef<SHorizontalBox> ParamBox = SNew(SHorizontalBox);
		auto AddParam = [&](const FString& InName, float MinV, float MaxV, float Delta,
			TFunction<float()> Get, TFunction<void(float)> Set)
		{
			ParamBox->AddSlot().AutoWidth().Padding(4.f, 0.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(FText::FromString(InName)).Font(LabelFont)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBox).WidthOverride(90.f)
					[
						SNew(SSpinBox<float>)
							.MinValue(MinV).MaxValue(MaxV).Delta(Delta)
							.Value_Lambda([Get]() { return Get(); })
							.OnValueChanged_Lambda([this, Set](float V)
							{
								Set(V);
								RefreshPreview(false);
							})
					]
				]
			];
		};
		AddParam(TEXT("Duration"), 0.01f, 5.f, 0.01f,
			[this]() { return WorkingParams->CharAnimDuration; },
			[this](float V) { WorkingParams->CharAnimDuration = V; });
		AddParam(TEXT("Char Delay"), 0.f, 1.f, 0.005f,
			[this]() { return WorkingParams->Stagger; },
			[this](float V) { WorkingParams->Stagger = V; });
		AddParam(TEXT("Amplitude"), 0.f, 100.f, 0.5f,
			[this]() { return WorkingParams->Amplitude; },
			[this](float V) { WorkingParams->Amplitude = V; });
		AddParam(TEXT("Frequency"), 0.f, 20.f, 0.1f,
			[this]() { return WorkingParams->Frequency; },
			[this](float V) { WorkingParams->Frequency = V; });

		ParamBox->AddSlot().AutoWidth().Padding(8.f, 0.f, 0.f, 0.f).VAlign(VAlign_Bottom)
		[
			SNew(SCheckBox)
				.IsChecked_Lambda([this]()
				{
					return WorkingParams->bLoop ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				})
				.OnCheckStateChanged_Lambda([this](ECheckBoxState State)
				{
					WorkingParams->bLoop = (State == ECheckBoxState::Checked);
					RefreshPreview(false);
				})
				[
					SNew(STextBlock).Text(LOCTEXT("Loop", "Loop"))
				]
		];

		ChildSlot
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(12.f, 12.f, 12.f, 4.f)
			[
				SNew(SWrapBox)
					.UseAllottedSize(true)
					.InnerSlotPadding(FVector2D(6.f, 6.f))
					+ SWrapBox::Slot().VAlign(VAlign_Bottom)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Text(LOCTEXT("EffectLabel", "Effect")).Font(LabelFont)
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SBox).WidthOverride(190.f)
							[
								SAssignNew(EffectCombo, SComboBox<TSharedPtr<int32>>)
									.OptionsSource(&EffectOptions)
									.OnGenerateWidget_Lambda([](TSharedPtr<int32> Item)
									{
										return SNew(STextBlock).Text(
											StaticEnum<ETextAnimEffect>()->GetDisplayNameTextByIndex(*Item));
									})
									.OnSelectionChanged_Lambda([this](TSharedPtr<int32> Item, ESelectInfo::Type)
									{
										if (Item.IsValid())
										{
											*WorkingEffect = (ETextAnimEffect)StaticEnum<ETextAnimEffect>()
												->GetValueByIndex(*Item);
											RefreshPreview(true);
										}
									})
									[
										SNew(STextBlock).Text_Lambda([this]()
										{
											return StaticEnum<ETextAnimEffect>()->GetDisplayNameTextByValue(
												(int64)*WorkingEffect);
										})
									]
							]
						]
					]
					+ SWrapBox::Slot().VAlign(VAlign_Bottom).Padding(10.f, 0.f, 0.f, 0.f)
					[
						ParamBox
					]
					+ SWrapBox::Slot().VAlign(VAlign_Bottom).Padding(4.f, 0.f)
					[
						SNew(SButton)
							.OnClicked_Lambda([this]() { RefreshPreview(true); return FReply::Handled(); })
							[
								SNew(STextBlock).Text(LOCTEXT("Replay", "Replay"))
							]
					]
					+ SWrapBox::Slot().VAlign(VAlign_Bottom).Padding(6.f, 0.f, 0.f, 0.f)
					[
						SAssignNew(HelpButton, SButton)
							.ButtonStyle(FCoreStyle::Get(), "NoBorder")
							.ContentPadding(FMargin(6.f))
							.ToolTipText(LOCTEXT("HelpTip", "How to use"))
							.OnClicked(this, &STextAnimPresetShowroom::OnHelpButtonClicked)
							[
								SNew(SBox).WidthOverride(28.f).HeightOverride(28.f)
								[
									MakeHelpButtonContent()
								]
							]
					]
				]
			+ SVerticalBox::Slot().AutoHeight().Padding(12.f, 6.f, 12.f, 2.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Bottom)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(LOCTEXT("PreviewTextLabel", "Preview Text")).Font(LabelFont)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SAssignNew(PreviewInput, SEditableTextBox)
							.Text_Lambda([this]() { return PreviewContent; })
							.OnTextChanged_Lambda([this](const FText& New)
							{
								PreviewContent = New;
								if (PreviewText.IsValid())
								{
									PreviewText->SetText(PreviewContent);
								}
								RefreshPreview(true);
							})
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(10.f, 0.f, 0.f, 0.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(LOCTEXT("FontLabel", "Font")).Font(LabelFont)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SBox).WidthOverride(220.f)
						[
							SNew(SObjectPropertyEntryBox)
								.AllowedClass(UFont::StaticClass())
								.DisplayThumbnail(false)
								.ObjectPath_Lambda([this]()
								{
									return PreviewFontInfo.FontObject
										? PreviewFontInfo.FontObject->GetPathName()
										: FString();
								})
								.OnObjectChanged_Lambda([this](const FAssetData& Asset)
								{
									UFont* F = Cast<UFont>(Asset.GetAsset());
									PreviewFontInfo.FontObject = F; // keep current size
									if (PreviewText.IsValid())
									{
										PreviewText->SetFont(PreviewFontInfo);
									}
									RefreshPreview(true);
								})
						]
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(10.f, 0.f, 0.f, 0.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(LOCTEXT("SizeLabel", "Size")).Font(LabelFont)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SBox).WidthOverride(70.f)
						[
							SNew(SSpinBox<int32>)
								.MinValue(6).MaxValue(200)
								.Value_Lambda([this]() { return (int32)PreviewFontInfo.Size; })
								.OnValueChanged_Lambda([this](int32 NewVal)
								{
									PreviewFontInfo.Size = (float)NewVal;
									if (PreviewText.IsValid())
									{
										PreviewText->SetFont(PreviewFontInfo);
									}
									RefreshPreview(true);
								})
						]
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(12.f, 6.f)
			[
				SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.BorderBackgroundColor(FLinearColor(0.03f, 0.03f, 0.05f))
					.Padding(14.f)
				[
					SAssignNew(PreviewText, SAnimatedText)
						.Text(PreviewContent)
						.Font(PreviewFontInfo)
						.Effect(*WorkingEffect)
						.Params(*WorkingParams)
						.ShadowOffset(FVector2D(2.f, 2.f))
						.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.6f))
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(12.f, 6.f, 12.f, 2.f)
			[
				SNew(STextBlock)
					.Text(LOCTEXT("DragHint", "Drag a card below into the UMG Designer to place a configured Animated Text Block. Click a card to load it into the editor above."))
					.Font(NoteFont)
					.ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f))
					.AutoWrapText(true)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(12.f, 4.f)
			[
				SNew(SSeparator)
			]
			+ SVerticalBox::Slot().FillHeight(1.f).Padding(6.f, 2.f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SAssignNew(CardList, SVerticalBox)
				]
			]
		];

		PopulateCards();
		RefreshPreview(true);
	}

private:
	struct FBuiltInPreset
	{
		FText Name;
		ETextAnimEffect Effect;
		FTextAnimParams Params;
	};

	static TArray<FBuiltInPreset> GetBuiltInPresets()
	{
		auto Make = [](const TCHAR* InName, ETextAnimEffect E,
			TFunction<void(FTextAnimParams&)> Tune = nullptr)
		{
			FBuiltInPreset P{ FText::FromString(InName), E, FTextAnimParams() };
			P.Params.Effect = E;
			if (Tune) { Tune(P.Params); }
			return P;
		};
		return {
			Make(TEXT("Classic Typewriter"), ETextAnimEffect::Typewriter,
				[](FTextAnimParams& P) { P.Stagger = 0.06f; }),
			Make(TEXT("Soft Fade"), ETextAnimEffect::FadeIn,
				[](FTextAnimParams& P) { P.CharAnimDuration = 0.6f; P.Stagger = 0.03f; }),
			Make(TEXT("Pop Up"), ETextAnimEffect::SlideUp,
				[](FTextAnimParams& P) { P.Amplitude = 16.f; }),
			Make(TEXT("Ocean Wave"), ETextAnimEffect::Wave,
				[](FTextAnimParams& P) { P.Amplitude = 6.f; P.Frequency = 1.5f; }),
		};
	}

	void AddCard(TSharedRef<SVerticalBox> TargetList, const FText& InName, ETextAnimEffect InEffect,
		const FTextAnimParams& InParams, UTextAnimPreset* InAsset, const FText& InSubtitle)
	{
		const FSlateFontInfo CardFont = FCoreStyle::GetDefaultFontStyle("Regular", 16);
		const FSlateFontInfo NameFont = FCoreStyle::GetDefaultFontStyle("Bold", 10);
		const FSlateFontInfo SubFont = FCoreStyle::GetDefaultFontStyle("Regular", 8);

		TSharedPtr<SAnimatedText> CardPreview;
		TSharedRef<SPresetCard> Card = SNew(SPresetCard)
			.OnClicked(FOnPresetCardClicked::CreateLambda([this, InEffect, InParams]()
			{
				*WorkingEffect = InEffect;
				*WorkingParams = InParams;
				RefreshPreview(true);
			}))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(170.f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Text(InName).Font(NameFont)
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
								.Text(InSubtitle)
								.Font(SubFont)
								.ColorAndOpacity(FLinearColor(0.55f, 0.55f, 0.55f))
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(10.f, 0.f)
				[
					SAssignNew(CardPreview, SAnimatedText)
						.Text(PreviewContent)
						.Font(CardFont)
						.Effect(InEffect)
						.Params(InParams)
				]
			];

		TWeakObjectPtr<UTextAnimPreset> WeakAsset = InAsset;
		Card->MakeTemplate = [InName, InEffect, InParams, WeakAsset]()
		{
			TSharedRef<FTextAnimPresetTemplate> Template = MakeShared<FTextAnimPresetTemplate>();
			Template->Name = InName;
			Template->Effect = InEffect;
			Template->Params = InParams;
			Template->Asset = WeakAsset;
			return Template;
		};

		if (CardPreview.IsValid())
		{
			CardPreview->Play();
			CardWidgets.Add(CardPreview);

			// Non-looping effects (Typewriter/Fade/Slide) finish quickly and go
			// static; looping effects (Wave) run forever. Replaying on a timer
			// keeps every card equally alive/consistent in this catalog view.
			CardPreview->RegisterActiveTimer(2.5f, FWidgetActiveTimerDelegate::CreateLambda(
				[WeakPreview = TWeakPtr<SAnimatedText>(CardPreview)](double, float) -> EActiveTimerReturnType
				{
					if (TSharedPtr<SAnimatedText> Pinned = WeakPreview.Pin())
					{
						Pinned->Play();
						return EActiveTimerReturnType::Continue;
					}
					return EActiveTimerReturnType::Stop;
				}));
		}
		TargetList->AddSlot().AutoHeight().Padding(8.f, 4.f)[Card];
	}

	void PopulateCards()
	{
		CardList->ClearChildren();
		CardWidgets.Reset();

		const FSlateFontInfo SectionFont = FCoreStyle::GetDefaultFontStyle("Bold", 11);
		auto AddSection = [&](const FText& Title, TFunctionRef<void(TSharedRef<SVerticalBox>)> PopulateBody)
		{
			// Each section is a collapsible SExpandableArea so its cards can be
			// folded away; the header keeps the same tinted-band look as before.
			TSharedRef<SVerticalBox> Body = SNew(SVerticalBox);
			PopulateBody(Body);

			CardList->AddSlot().AutoHeight().Padding(4.f, 10.f, 4.f, 2.f)
			[
				SNew(SExpandableArea)
					.InitiallyCollapsed(false)
					.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.BorderBackgroundColor(FLinearColor(0.06f, 0.08f, 0.10f, 1.f))
					.HeaderPadding(FMargin(10.f, 6.f))
					.BodyBorderBackgroundColor(FLinearColor::Transparent)
					.Padding(0.f)
					.HeaderContent()
					[
						SNew(STextBlock).Text(Title).Font(SectionFont)
							.ColorAndOpacity(FLinearColor(0.5f, 0.7f, 1.f))
					]
					.BodyContent()
					[
						Body
					]
			];
		};

		AddSection(LOCTEXT("BuiltInSection", "Default Presets"), [&](TSharedRef<SVerticalBox> Body)
		{
			for (const FBuiltInPreset& P : GetBuiltInPresets())
			{
				AddCard(Body, P.Name, P.Effect, P.Params, nullptr,
					StaticEnum<ETextAnimEffect>()->GetDisplayNameTextByValue((int64)P.Effect));
			}
		});

		AddSection(LOCTEXT("AssetSection", "Project Presets (TextAnimPreset assets)"), [&](TSharedRef<SVerticalBox> Body)
		{
			const FAssetRegistryModule& AssetRegistry =
				FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
			TArray<FAssetData> Assets;
			AssetRegistry.Get().GetAssetsByClass(UTextAnimPreset::StaticClass()->GetClassPathName(), Assets);
			for (const FAssetData& Data : Assets)
			{
				if (UTextAnimPreset* Preset = Cast<UTextAnimPreset>(Data.GetAsset()))
				{
					AddCard(Body, FText::FromName(Data.AssetName), Preset->Effect, Preset->Params, Preset,
						FText::FromString(Data.PackagePath.ToString()));
				}
			}
		});
	}

	// Breathing cyan glow driven by the looping pulse curve; catches the eye
	// without being harsh. Shared by both the icon and text-fallback branches.
	FLinearColor GetHelpPulseColor() const
	{
		const float T = HelpPulseCurve.GetLerp();                    // 0..1 looping
		const float Pulse = 0.5f + 0.5f * FMath::Sin(T * 2.f * PI);  // 0..1 smooth
		return FLinearColor(0.35f, 0.75f, 1.0f) * FMath::Lerp(0.7f, 1.4f, Pulse);
	}

	TSharedRef<SWidget> MakeHelpButtonContent()
	{
		const ISlateStyle* Style = FSlateStyleRegistry::FindSlateStyle(TEXT("TextAnimatorStyle"));
		const FSlateBrush* InfoBrush = Style ? Style->GetOptionalBrush(TEXT("TextAnimator.InfoIcon")) : nullptr;
		if (InfoBrush)
		{
			return SNew(SBox).WidthOverride(16.f).HeightOverride(16.f)
			[
				SNew(SImage)
					.Image(InfoBrush)
					.ColorAndOpacity_Lambda([this]() { return GetHelpPulseColor(); })
			];
		}
		return SNew(STextBlock)
			.Text(LOCTEXT("HelpIcon", " i "))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
			.ColorAndOpacity_Lambda([this]() { return FSlateColor(GetHelpPulseColor()); });
	}

	FReply OnHelpButtonClicked()
	{
		if (!HelpButton.IsValid())
		{
			return FReply::Handled();
		}

		FSlateApplication::Get().PushMenu(
			HelpButton.ToSharedRef(),
			FWidgetPath(),
			MakeHelpContent(),
			FSlateApplication::Get().CalculatePopupWindowPosition(
				HelpButton->GetCachedGeometry().GetLayoutBoundingRect(),
				FVector2D(420.f, 400.f)),
			FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
		return FReply::Handled();
	}

	TSharedRef<SWidget> MakeHelpContent()
	{
		const FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 11);
		const FSlateFontInfo BodyFont = FCoreStyle::GetDefaultFontStyle("Regular", 9);
		auto Section = [&](TSharedRef<SVerticalBox> Box, const FText& Head, const FText& Body)
		{
			Box->AddSlot().AutoHeight().Padding(2.f, 8.f, 2.f, 2.f)
			[ SNew(STextBlock).Text(Head).Font(TitleFont).ColorAndOpacity(FLinearColor(0.5f, 0.7f, 1.f)) ];
			Box->AddSlot().AutoHeight().Padding(2.f)
			[ SNew(STextBlock).Text(Body).Font(BodyFont).ColorAndOpacity(FLinearColor(0.85f, 0.85f, 0.85f)).AutoWrapText(true) ];
		};

		TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
		Section(Box, LOCTEXT("HelpBasicHead", "Basic use"),
			LOCTEXT("HelpBasicBody",
				"1. In a Widget Blueprint, drag 'Animated Text Block' from the Palette onto the canvas.\n\n"
				"2. Select it. In Details, type your Text and choose an Effect. Adjust the Params shown.\n\n"
				"3. It plays automatically when the widget appears. Turn on 'Preview In Designer' to watch while editing.\n\n"
				"4. From Blueprints: Activate replays, SetText changes text, SkipToEnd finishes instantly, and On Animation Finished fires when done.\n\n"
				"5. Reuse a look: drag a preset card below into the Designer, or assign a Preset asset to the widget's Preset slot.\n\n"
				"6. Faster way: open Window > Text Animator Presets to launch this Showroom, then drag any preset card straight into your Widget Blueprint's Designer canvas."));

		return SNew(SBox).WidthOverride(420.f).Padding(12.f)
		[
			SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(0.03f, 0.03f, 0.05f))
				.Padding(10.f)
			[
				Box
			]
		];
	}

	void RefreshPreview(bool bReplay)
	{
		if (PreviewText.IsValid())
		{
			PreviewText->SetEffect(*WorkingEffect);
			PreviewText->SetParams(*WorkingParams);
			if (bReplay)
			{
				PreviewText->Play();
			}
		}
		// Keep every preset card's mini preview showing the live preview text.
		// Don't call Play() here — updating text mid-keystroke shouldn't restart
		// each card's looping animation.
		for (const TSharedPtr<SAnimatedText>& Card : CardWidgets)
		{
			if (Card.IsValid())
			{
				Card->SetText(PreviewContent);
			}
		}
	}

	TSharedPtr<ETextAnimEffect> WorkingEffect;
	TSharedPtr<FTextAnimParams> WorkingParams;
	TArray<TSharedPtr<int32>> EffectOptions;
	TSharedPtr<SComboBox<TSharedPtr<int32>>> EffectCombo;
	TSharedPtr<SAnimatedText> PreviewText;
	TSharedPtr<class SEditableTextBox> PreviewInput;
	FText PreviewContent = FText::FromString(TEXT("The quick brown fox"));
	FSlateFontInfo PreviewFontInfo; // initialized in Construct
	TSharedPtr<SVerticalBox> CardList;
	TArray<TSharedPtr<SAnimatedText>> CardWidgets;
	TSharedPtr<SButton> HelpButton;
	FCurveSequence HelpPulseCurve;
};

// ---------------------------------------------------------------------------
// Module + tab registration

class FTextAnimatorEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		RegisterStyle();

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TextAnimShowroomTabId,
			FOnSpawnTab::CreateStatic(&FTextAnimatorEditorModule::SpawnShowroomTab))
			.SetDisplayName(LOCTEXT("TabTitle", "Text Animator Presets"))
			.SetTooltipText(LOCTEXT("TabTooltip", "Preset showroom: preview and drag presets into the UMG Designer."))
			.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory())
			.SetIcon(FSlateIcon(TEXT("TextAnimatorStyle"), TEXT("TextAnimator.TabIcon")));

		// Auto-open the Text Test window once the editor main frame is ready.
		IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
		if (MainFrame.IsWindowInitialized())
		{
			MaybeOpenWelcome();
		}
		else
		{
			MainFrame.OnMainFrameCreationFinished().AddRaw(this, &FTextAnimatorEditorModule::OnMainFrameReady);
		}
	}

	virtual void ShutdownModule() override
	{
		if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
		{
			IMainFrameModule::Get().OnMainFrameCreationFinished().RemoveAll(this);
		}

		if (FSlateApplication::IsInitialized())
		{
			FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TextAnimShowroomTabId);
		}

		if (StyleSet.IsValid())
		{
			FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet);
			StyleSet.Reset();
		}
	}

private:
	// Style set holding the plugin icon so the Window-menu tab entry shows it.
	TSharedPtr<FSlateStyleSet> StyleSet;

	void RegisterStyle()
	{
		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("TextAnimator"));
		if (!Plugin.IsValid())
		{
			return;
		}
		const FString IconPath = Plugin->GetBaseDir() / TEXT("Resources/Icon128.png");
		if (!FPaths::FileExists(IconPath))
		{
			return;
		}
		StyleSet = MakeShareable(new FSlateStyleSet(TEXT("TextAnimatorStyle")));
		StyleSet->Set(TEXT("TextAnimator.TabIcon"),
			new FSlateImageBrush(IconPath, FVector2D(16.f, 16.f)));

		const FString InfoPath = Plugin->GetBaseDir() / TEXT("Resources/InfoIcon.svg");
		if (FPaths::FileExists(InfoPath))
		{
			StyleSet->Set(TEXT("TextAnimator.InfoIcon"),
				new FSlateVectorImageBrush(InfoPath, FVector2D(16.f, 16.f)));
		}

		FSlateStyleRegistry::RegisterSlateStyle(*StyleSet);
	}

	void OnMainFrameReady(TSharedPtr<SWindow>, bool)
	{
		MaybeOpenWelcome();
	}

	void MaybeOpenWelcome()
	{
		bool bShow = true;
		GConfig->GetBool(TEXT("EasyTextAnimation"), TEXT("ShowWelcomeOnStartup"), bShow, GEditorPerProjectIni);
		if (bShow && GEngine)
		{
			GEngine->Exec(nullptr, TEXT("TextAnimator.Test"));
		}
	}

	static TSharedRef<SDockTab> SpawnShowroomTab(const FSpawnTabArgs& Args)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			[
				SNew(STextAnimPresetShowroom)
			];
	}
};

IMPLEMENT_MODULE(FTextAnimatorEditorModule, TextAnimatorEditor);

#undef LOCTEXT_NAMESPACE
