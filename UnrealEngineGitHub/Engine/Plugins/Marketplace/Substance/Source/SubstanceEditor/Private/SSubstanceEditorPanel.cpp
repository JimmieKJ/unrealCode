// Copyright 2016 Allegorithmic Inc. All rights reserved.
// File: SSubstanceEditorPanel.cpp

#include "SubstanceEditorPrivatePCH.h"
#include "SubstanceEditorModule.h"
#include "SSubstanceEditorPanel.h"
#include "../../SubstanceCore/Classes/SubstanceGraphInstance.h"

#include "PropertyEditing.h"
#include "PropertyCustomizationHelpers.h"
#include "ContentBrowserModule.h"
#include "MouseDeltaTracker.h"
#include "Slate/SceneViewport.h"
#include "SColorPicker.h"
#include "SNumericEntryBox.h"
#include "SExpandableArea.h"
#include "ScopedTransaction.h"
#include "Editor/LevelEditor/Public/LevelEditor.h"
#include "SAssetDropTarget.h"
#include "SubstanceCoreModule.h"

#include "substance/framework/graph.h"
#include "substance/framework/input.h"
#include "substance/framework/typedefs.h"

#include "../../SubstanceCore/Public/SubstanceInput.inl"

#define LOC_NAMESPACE TEXT("SubstanceEditor")

SSubstanceEditorPanel::~SSubstanceEditorPanel()
{
}

TWeakPtr<ISubstanceEditor> SSubstanceEditorPanel::GetSubstanceEditor() const
{
	return SubstanceEditorPtr;
}

void SSubstanceEditorPanel::OnRedo()
{
	TMap<TSharedPtr<FAssetThumbnail>, SubstanceAir::InputInstanceImage*>::TIterator ItBegin(ThumbnailInputs);

	while (ItBegin)
	{
		ItBegin.Key()->SetAsset(reinterpret_cast<InputImageData*>(ItBegin.Value()->getImage().get()->mUserData)->ImageUObjectSource);
		++ItBegin;
	}
}

void SSubstanceEditorPanel::OnUndo()
{
	TMap<TSharedPtr<FAssetThumbnail>, SubstanceAir::InputInstanceImage*>::TIterator ItBegin(ThumbnailInputs.CreateIterator());

	while (ItBegin)
	{
		ItBegin.Key()->SetAsset(reinterpret_cast<InputImageData*>(ItBegin.Value()->getImage().get()->mUserData)->ImageUObjectSource);
		++ItBegin;
	}
}

void SSubstanceEditorPanel::Construct(const FArguments& InArgs)
{
	SubstanceEditorPtr = InArgs._SubstanceEditor;

	OutputSizePow2Min = 5;
	OutputSizePow2Max = FMath::Log2(FModuleManager::GetModuleChecked<ISubstanceCore>("SubstanceCore").GetMaxOutputTextureSize());

	Graph = SubstanceEditorPtr.Pin()->GetGraph();

	if (!Graph->ParentFactory || !Graph->Instance)
	{
		UE_LOG(LogSubstanceEditor, Error, TEXT("Invalid Substance Graph Instance, cannot edit the instance."));
		return;
	}

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	ThumbnailPool = LevelEditorModule.GetFirstLevelEditor()->GetThumbnailPool();

	ConstructDescription();
	ConstructOutputs();
	ConstructInputs();

	TSharedPtr<SVerticalBox> ChildWidget =
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 3.0f)
		[
			DescArea.ToSharedRef()
		]
	+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 3.0f)
		[
			OutputsArea.ToSharedRef()
		];

	if (InputsArea.IsValid())
	{
		ChildWidget->AddSlot()
			.AutoHeight()
			.Padding(0.0f, 3.0f)
			[
				InputsArea.ToSharedRef()
			];
	}

	if (ImageInputsArea.IsValid())
	{
		ChildWidget->AddSlot()
			.AutoHeight()
			.Padding(0.0f, 3.0f)
			[
				ImageInputsArea.ToSharedRef()
			];
	}

	ChildSlot
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
		.Padding(0.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f)
		[
			ChildWidget.ToSharedRef()
		]
		]
		];
}

void SSubstanceEditorPanel::ConstructDescription()
{
	DescArea = SNew(SExpandableArea)
		.AreaTitle(FText::FromString(TEXT("Graph Description:")))
		.InitiallyCollapsed(false)
		.BodyContent()
		[
			SNew(SBorder)
			.Content()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
		.Padding(0.1f)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.FillWidth(0.3f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Label:")))
		]
	+ SHorizontalBox::Slot()
		.Padding(0.1f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Graph->Instance->mDesc.mLabel.c_str()))
		]
		]
	+ SVerticalBox::Slot()
		.Padding(0.1f)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.FillWidth(0.3f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Description:")))
		]
	+ SHorizontalBox::Slot()
		.Padding(0.1f)
		[
			SNew(STextBlock)
			.AutoWrapText(true)
		.Text(Graph->Instance->mDesc.mDescription.length() ? FText::FromString(Graph->Instance->mDesc.mDescription.c_str()) : FText::FromString(TEXT("N/A")))
		]
		]
	+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 3.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.FillWidth(0.3f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Runtime modifications:")))
		]
	+ SHorizontalBox::Slot()
		.Padding(0.1f)
		[
			SNew(SCheckBox)
			.OnCheckStateChanged(this, &SSubstanceEditorPanel::OnFreezeGraphValueChanged)
		.IsChecked(this, &SSubstanceEditorPanel::GetFreezeGraphValue)
		.Content()
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Disable")))
		]
		]
		]
		]
		];
}

void SSubstanceEditorPanel::ConstructOutputs()
{
	TSharedPtr<SVerticalBox> OutputsBox = SNew(SVerticalBox);

	for (int32 i = 0; i < Graph->Instance->getOutputs().size(); ++i)
	{
		FReferencerInformationList Refs;
		UObject* TextureObject = NULL;
		if (Graph->Instance->getOutputs()[i]->mUserData != 0)
			TextureObject = reinterpret_cast<OutputInstanceData*>(Graph->Instance->getOutputs()[i]->mUserData)->Texture.Get();

		// determine whether the transaction buffer is the only thing holding a reference to the object
		// and if so, behave like it's not referenced and ask offer the user to clean up undo/redo history when he deletes the output
		GEditor->Trans->DisableObjectSerialization();
		bool bIsReferenced = TextureObject ? IsReferenced(TextureObject, GARBAGE_COLLECTION_KEEPFLAGS, EInternalObjectFlags::GarbageCollectionKeepFlags, true, &Refs) : false;
		GEditor->Trans->EnableObjectSerialization();

		const SubstanceAir::OutputDesc* OutputDesc = &Graph->Instance->getOutputs()[i]->mDesc;

		OutputsBox->AddSlot()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
			.FillWidth(0.3f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
			[
				SNew(STextBlock)
				.Text(OutputDesc ? FText::FromString(OutputDesc->mLabel.c_str()) : FText::FromString(TEXT("Invalid output")))
			]
		+ SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			[
				PropertyCustomizationHelpers::MakeBrowseButton(
					FSimpleDelegate::CreateSP(this, &SSubstanceEditorPanel::OnBrowseTexture, Graph->Instance->getOutputs()[i]))
			]
			]
		+ SHorizontalBox::Slot()
			[
				SNew(SCheckBox)
				.OnCheckStateChanged(this, &SSubstanceEditorPanel::OnToggleOutput, Graph->Instance->getOutputs()[i])
			.IsChecked(this, &SSubstanceEditorPanel::GetOutputState, Graph->Instance->getOutputs()[i])
			.IsEnabled(!bIsReferenced && NULL != OutputDesc)
			.ToolTipText(FText::FromString(OutputDesc->mIdentifier.c_str()))
			]
			];
	}

	OutputsArea = SNew(SExpandableArea)
		.AreaTitle(FText::FromString(TEXT("Outputs:")))
		.InitiallyCollapsed(false)
		.BodyContent()
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.35, 0.35, 0.35))
		.ForegroundColor(FEditorStyle::GetColor("TableView.Header", ".Foreground"))
		.Content()
		[
			OutputsBox.ToSharedRef()
		]
		];
}

void ActualizeMaterialGraph(SubstanceAir::GraphInstance* Instance)
{
	for (int32 i = 0; i < Instance->getOutputs().size(); ++i)
	{
		if (Instance->getOutputs()[i]->mUserData == 0)
			continue;

		FReferencerInformationList Refs;
		UObject* TextureObject = reinterpret_cast<OutputInstanceData*>(Instance->getOutputs()[i]->mUserData)->Texture.Get();

		// Check and see whether we are referenced by any objects that won't be garbage collected.
		bool bIsReferenced = TextureObject ? IsReferenced(TextureObject, GARBAGE_COLLECTION_KEEPFLAGS, EInternalObjectFlags::GarbageCollectionKeepFlags, true, &Refs) : false;

		TArray<UMaterial*> RefMaterials;

		TArray<FReferencerInformation>::TIterator ItRef(Refs.ExternalReferences);
		for (; ItRef; ++ItRef)
		{
			UMaterial* Material = Cast<UMaterial>(ItRef->Referencer);

			if (Material && Material->MaterialGraph)
			{
				RefMaterials.AddUnique(Material);
			}
		}

		TArray<UMaterial*>::TIterator ItMat(RefMaterials);
		for (; ItMat; ++ItMat)
		{
			(*ItMat)->MaterialGraph->RebuildGraph();
		}
	}
}

void SSubstanceEditorPanel::OnToggleOutput(ECheckBoxState InNewState, SubstanceAir::OutputInstance* Output)
{
	UTexture* Texture = NULL;

	if (Output->mUserData != 0 &&
		reinterpret_cast<OutputInstanceData*>(Output->mUserData)->Texture.Get())
		Texture = reinterpret_cast<OutputInstanceData*>(Output->mUserData)->Texture.Get();

	if (InNewState == ECheckBoxState::Checked)
	{
		FString TextureName;
		FString PackageName;
		Substance::Helpers::GetSuitableName(Output, TextureName, PackageName, Graph);
		UObject* TextureParent = CreatePackage(NULL, *PackageName);

		Substance::Helpers::CreateSubstanceTexture2D(Output, false, TextureName, TextureParent, Graph);

		TArray<SubstanceAir::GraphInstance*> Instances;
		Instances.Push(reinterpret_cast<OutputInstanceData*>(Output->mUserData)->ParentInstance->Instance);
		Substance::Helpers::RenderAsync(Instances);

		TArray<UObject*> NewTexture;
		NewTexture.Add(reinterpret_cast<OutputInstanceData*>(Output->mUserData)->Texture.Get());

		FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ContentBrowserModule.Get().SyncBrowserToAssets(NewTexture);

		// should rebuild the graph of any material using a substance output
		ActualizeMaterialGraph(Graph->Instance);
	}
	else if (InNewState == ECheckBoxState::Unchecked && Texture && Texture->IsValidLowLevel())
	{
		FReferencerInformationList Refs;
		UObject* TextureObject = reinterpret_cast<OutputInstanceData*>(Output->mUserData)->Texture.Get();

		// Check and see whether we are referenced by any objects that won't be garbage collected.
		bool bIsReferenced = TextureObject ? IsReferenced(TextureObject, GARBAGE_COLLECTION_KEEPFLAGS, EInternalObjectFlags::GarbageCollectionKeepFlags, true, &Refs) : false;

		if (bIsReferenced)
		{
			// determine whether the transaction buffer is the only thing holding a reference to the object
			// and if so, behave like it's not referenced and ask offer the user to clean up undo/redo history when he deletes the output
			GEditor->Trans->DisableObjectSerialization();
			bIsReferenced = IsReferenced(TextureObject, GARBAGE_COLLECTION_KEEPFLAGS, EInternalObjectFlags::GarbageCollectionKeepFlags, true, &Refs);
			GEditor->Trans->EnableObjectSerialization();

			// only ref to this object is the transaction buffer - let the user choose whether to clear the undo buffer
			if (!bIsReferenced)
			{
				if (EAppReturnType::Yes == FMessageDialog::Open(
					EAppMsgType::YesNo, NSLOCTEXT("UnrealEd", "ResetUndoBufferForObjectDeletionPrompt",
						"The only reference to this object is the undo history.  In order to delete this object, you must clear all undo history - would you like to clear undo history?")))
				{
					GEditor->Trans->Reset(NSLOCTEXT("UnrealEd", "DeleteSelectedItem", "Delete Selected Item"));
				}
				else
				{
					bIsReferenced = true;
				}
			}
		}

		if (reinterpret_cast<OutputInstanceData*>(Output->mUserData)->Texture.Get() && !bIsReferenced)
		{
			Substance::Helpers::RegisterForDeletion(reinterpret_cast<OutputInstanceData*>(Output->mUserData)->Texture.Get());
		}
		else
		{
			FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("UnrealEd", "ObjectStillReferenced", "The texture is in use, it cannot be deleted."));
		}

		Substance::Helpers::Tick(); // need to make sure the texture has been deleted completely
		ActualizeMaterialGraph(Graph->Instance);
	}
}

ECheckBoxState SSubstanceEditorPanel::GetOutputState(SubstanceAir::OutputInstance* Output) const
{
	ECheckBoxState CurrentState;
	(Output->mUserData != 0) ? CurrentState = ECheckBoxState::Checked : CurrentState = ECheckBoxState::Unchecked;
	return CurrentState;
}

void SSubstanceEditorPanel::OnBrowseTexture(SubstanceAir::OutputInstance* Output)
{
	if (reinterpret_cast<OutputInstanceData*>(Output->mUserData)->Texture.Get())
	{
		TArray<UObject*> Objects;
		Objects.Add(reinterpret_cast<OutputInstanceData*>(Output->mUserData)->Texture.Get());
		GEditor->SyncBrowserToObjects(Objects);
	}
}

void SSubstanceEditorPanel::OnBrowseImageInput(SubstanceAir::InputInstanceImage* ImageInput)
{
	if (ImageInput->getImage())
	{
		TArray<UObject*> Objects;
		Objects.Add(reinterpret_cast<InputImageData*>(ImageInput->getImage().get()->mUserData)->ImageUObjectSource);
		GEditor->SyncBrowserToObjects(Objects);
	}
}

TSharedRef<SHorizontalBox> SSubstanceEditorPanel::GetBaseInputWidget(SubstanceAir::InputInstanceBase* Input, FString InputLabel)
{
	if (0 == InputLabel.Len())
	{
		InputLabel = Input->mDesc.mLabel.c_str();
	}

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		[
			SNew(STextBlock)
			.Text(FText::FromString(InputLabel))
		.ToolTipText(FText::FromString(Input->mDesc.mIdentifier.c_str()))
		.Font(FEditorStyle::GetFontStyle("BoldFont"))
		]
	+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.ToolTipText(NSLOCTEXT("PropertyEditor", "ResetToDefaultToolTip", "Reset to Default"))
		.ButtonStyle(FEditorStyle::Get(), "NoBorder")
		.OnClicked(this, &SSubstanceEditorPanel::OnResetInput, Input)
		.Content()
		[
			SNew(SImage)
			.Image(FEditorStyle::GetBrush("PropertyWindow.DiffersFromDefault"))
		]
		];
}

void SSubstanceEditorPanel::ConstructInputs()
{
	TSharedPtr<SScrollBox> InputsBox = TSharedPtr<SScrollBox>(NULL);
	TSharedPtr<SScrollBox> ImageInputsBox = TSharedPtr<SScrollBox>(NULL);

	InputsArea = TSharedPtr<SExpandableArea>(NULL);
	ImageInputsArea = TSharedPtr<SExpandableArea>(NULL);

	TMap< FString, TSharedPtr< SVerticalBox > > Groups;

	// make a copy of the list of inputs to sort them by index
	std::vector<SubstanceAir::InputInstanceBase*> IdxSortedInputs;

	for (auto InpItr = Graph->Instance->getInputs().begin(); InpItr != Graph->Instance->getInputs().end(); ++InpItr)
	{
		SubstanceAir::InputInstanceBase* Sptr(*InpItr);
		IdxSortedInputs.push_back(Sptr);
	}

	struct FCompareInputIdx
	{
		FORCEINLINE bool operator()(const SubstanceAir::InputInstanceBase* A, const SubstanceAir::InputInstanceBase* B) const
		{
			return atoi(A->mDesc.mUserTag.c_str()) < atoi(B->mDesc.mUserTag.c_str());
		}
	};

	std::sort(IdxSortedInputs.begin(), IdxSortedInputs.begin() + Graph->Instance->getInputs().size(), FCompareInputIdx());

	for (int32 i = 0; i < IdxSortedInputs.size(); ++i)
	{
		SubstanceAir::InputInstanceBase* ItIn = IdxSortedInputs[i];

		if (ItIn->mDesc.mIdentifier == "$pixelsize" ||
			ItIn->mDesc.mIdentifier == "$time" ||
			ItIn->mDesc.mIdentifier == "$normalformat")
		{
			continue;
		}

		if (ItIn->mDesc.isNumerical())
		{
			if (false == InputsBox.IsValid())
			{
				InputsBox = SNew(SScrollBox)
					.IsEnabled(this, &SSubstanceEditorPanel::GetInputsEnabled);

				InputsArea = SNew(SExpandableArea)
					.AreaTitle(FText::FromString(TEXT("Inputs:")))
					.InitiallyCollapsed(false)
					.BodyContent()
					[
						SNew(SBorder)
						.Content()
					[
						InputsBox.ToSharedRef()
					]
					];
			}

			FString GroupName = ItIn->mDesc.mGuiGroup.c_str();

			if (GroupName.Len())
			{
				TSharedPtr<SVerticalBox> GroupBox;

				if (Groups.Contains(GroupName))
				{
					GroupBox = *Groups.Find(GroupName);
				}
				else
				{
					GroupBox = SNew(SVerticalBox);
					Groups.Add(GroupName, GroupBox);
				}

				GroupBox->AddSlot()
					[
						SNew(SSeparator)
						.Orientation(EOrientation::Orient_Horizontal)
					];

				GroupBox->AddSlot()
					.AutoHeight()
					.Padding(10.0f, 0.0f)
					[
						GetInputWidget(ItIn)
					];
			}
			else
			{
				InputsBox->AddSlot()
					[
						GetInputWidget(ItIn)
					];

				InputsBox->AddSlot()
					[
						SNew(SSeparator)
						.Orientation(EOrientation::Orient_Horizontal)
					];
			}
		}
		else
		{
			if (false == ImageInputsBox.IsValid())
			{
				ImageInputsBox = SNew(SScrollBox);

				ImageInputsArea = SNew(SExpandableArea)
					.AreaTitle(FText::FromString(TEXT("Image Inputs:")))
					.InitiallyCollapsed(false)
					.BodyContent()
					[
						SNew(SBorder)
						.Content()
					[
						ImageInputsBox.ToSharedRef()
					]
					];
			}

			ImageInputsBox->AddSlot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
				[
					GetImageInputWidget(ItIn)
				]
				];
		}
	}

	TMap< FString, TSharedPtr< SVerticalBox >>::TIterator ItGroups(Groups);

	FLinearColor OddColor(.35, .35, .35, 0.0f);
	FLinearColor EvenColor(.4, .4, .4, 0.0f);

	int32 Idx = 0;

	for (; ItGroups; ++ItGroups)
	{
		InputsBox->AddSlot()
			[
				SNew(SExpandableArea)
				.AreaTitle(FText::FromString(ItGroups.Key()))
			.InitiallyCollapsed(false)
			.BodyContent()
			[
				ItGroups.Value().ToSharedRef()
			]
			];

		InputsBox->AddSlot()
			[
				SNew(SSeparator)
				.Orientation(EOrientation::Orient_Horizontal)
			];
	}
}

void SSubstanceEditorPanel::OnFreezeGraphValueChanged(ECheckBoxState InNewState)
{
	check(Graph);

	if (Graph)
	{
		Graph->bIsFrozen = ECheckBoxState::Checked == InNewState ? true : false;

		Graph->Modify();
	}
}

ECheckBoxState SSubstanceEditorPanel::GetFreezeGraphValue() const
{
	if (Graph)
	{
		return Graph->bIsFrozen ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}

	return ECheckBoxState::Unchecked;
}

template< typename T> void SSubstanceEditorPanel::SetValue(T NewValue, SubstanceAir::InputInstanceBase* Input, int32 Index)
{
	FInputValue<T> value = { NewValue, Input, Index };
	SetValues(1, &value);
}

template<typename T> void SSubstanceEditorPanel::SetValues(uint32 NumInputValues, FInputValue<T>* Inputs)
{
	bool renderAsync = false;

	SubstanceAir::InputInstanceNumericalBase* InputInst = (SubstanceAir::InputInstanceNumericalBase*)Inputs[0].Input;
	TArray<T> NewValueSet;
	Substance::GetNumericalInputValue<T>(InputInst, NewValueSet);

	//Early out
	if (NewValueSet.Num() == 0 || NumInputValues == 0)
	{
		UE_LOG(LogSubstanceEditor, Warning, TEXT("Attempted to update an invalid input parameter"));
		return;
	}

	for (uint32 i = 0; i < NumInputValues; i++)
	{
		FInputValue<T>& currentInput = Inputs[i];

		//Make sure the value has changed
		if (NewValueSet[currentInput.Index] == currentInput.NewValue)
		{
			continue;
		}

		NewValueSet[currentInput.Index] = currentInput.NewValue;
		renderAsync = true;
	}

	Substance::Helpers::SetNumericalInputValue(InputInst, NewValueSet);
	Substance::Helpers::UpdateInput(Graph->Instance, InputInst->mDesc.mUid, NewValueSet);

	if (renderAsync)
	{
		TArray< SubstanceAir::GraphInstance* > Graphs;
		Graphs.AddUnique(Graph->Instance);
		Substance::Helpers::RenderAsync(Graphs);
		Graph->MarkPackageDirty();
	}
}

template<typename T> TOptional<T> SSubstanceEditorPanel::GetInputValue(SubstanceAir::InputInstanceBase* Input, int32 Index) const
{
	SubstanceAir::InputInstanceNumerical<T>* TypedInst = (SubstanceAir::InputInstanceNumerical<T>*)Input;

	TArray<T> Values;
	Substance::GetNumericalInputValue<T>((SubstanceAir::InputInstanceNumericalBase*)Input, Values);
	return Values[Index];
}

FText SSubstanceEditorPanel::GetInputValue(SubstanceAir::InputInstanceBase* Input) const
{
	SubstanceAir::InputInstanceInt* TypedInst = (SubstanceAir::InputInstanceInt*)Input;
	SubstanceAir::InputDescInt* TypedDesc = (SubstanceAir::InputDescInt*)&TypedInst->mDesc;

	if (TypedDesc->mEnumValues.size() >= 1)
	{
		return FText::FromString(TypedDesc->mEnumValues[TypedInst->getValue()].second.c_str());
	}
	else
	{
		return FText();
	}
}

TSharedRef<SWidget> SSubstanceEditorPanel::GetInputWidgetCombobox(SubstanceAir::InputInstanceBase* Input)
{
	SubstanceAir::InputDescInt* TypedDesc = (SubstanceAir::InputDescInt*)&Input->mDesc;

	SharedFStringArray* ItemsForWidget = new SharedFStringArray();
	for (auto ValItr = TypedDesc->mEnumValues.begin(); ValItr != TypedDesc->mEnumValues.end(); ++ValItr)
	{
		ItemsForWidget->Add(MakeShareable(new FString(ValItr->second.c_str())));
	}

	ComboBoxLabels.Add(MakeShareable(ItemsForWidget));
	FString DebugStrTest = FString(GetInputValue(Input).ToString());
	TSharedPtr< FString > CurrentValue = MakeShareable(new FString(GetInputValue(Input).ToString()));

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(0.3f)
		[
			GetBaseInputWidget(Input)
		]
	+ SHorizontalBox::Slot()
		[
			SNew(SComboBox< TSharedPtr<FString> >)
			.OptionsSource(ComboBoxLabels.Last().Get())
		.OnGenerateWidget(this, &SSubstanceEditorPanel::MakeInputComboWidget)
		.OnSelectionChanged(this, &SSubstanceEditorPanel::OnComboboxSelectionChanged, Input)
		.InitiallySelectedItem(CurrentValue)
		.ContentPadding(0)
		.Content()
		[
			SNew(STextBlock)
			.Text(this, &SSubstanceEditorPanel::GetInputValue, Input)
		]
		];
}

TSharedRef<SWidget> SSubstanceEditorPanel::MakeInputComboWidget(TSharedPtr<FString> InItem)
{
	return SNew(STextBlock).Text(FText::FromString(*InItem));
}

bool SSubstanceEditorPanel::OnAssetDraggedOver(const UObject* InObject) const
{
	if (InObject && Cast<USubstanceImageInput>(InObject))
	{
		return true;
	}

	return false;
}

void SSubstanceEditorPanel::OnAssetDropped(UObject* InObject, SubstanceAir::InputInstanceBase* Input, TSharedPtr<FAssetThumbnail> Thumbnail)
{
	OnSetImageInput(InObject, Input, Thumbnail);
}

void SSubstanceEditorPanel::OnComboboxSelectionChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo, SubstanceAir::InputInstanceBase* Input/*, TSharedRef<STextBlock> ComboText*/)
{
	SubstanceAir::InputDescInt* TypedDesc = (SubstanceAir::InputDescInt*)&Input->mDesc;

	if (!Item.IsValid() || TypedDesc->mEnumValues.size() == 0)
	{
		return;
	}

	auto EnItr = TypedDesc->mEnumValues.begin();
	for (; EnItr != TypedDesc->mEnumValues.end(); ++EnItr)
	{
		if (EnItr->second.c_str() == *Item)
		{
			float ItemValue((float)EnItr->first + 0.1f);
			FInputValue<float> value = { ItemValue, Input, 0 };
			SetValues<float>(1, &value);
		}
	}
}

TSharedRef<SWidget> SSubstanceEditorPanel::GetInputWidgetTogglebutton(SubstanceAir::InputInstanceBase* Input)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(0.3f)
		[
			GetBaseInputWidget(Input)
		]
	+ SHorizontalBox::Slot()
		[
			SNew(SCheckBox)
			.OnCheckStateChanged(this, &SSubstanceEditorPanel::OnToggleValueChanged, Input)
		.IsChecked(this, &SSubstanceEditorPanel::GetToggleValue, Input)
		];
}

void SSubstanceEditorPanel::OnToggleValueChanged(ECheckBoxState InNewState, SubstanceAir::InputInstanceBase* Input)
{
	FScopedTransaction Transaction(NSLOCTEXT("SubstanceEditor", "SubstanceSetInput", "Substance set input"));
	Graph->Modify();

	FInputValue<int32> value = { 1, Input, 0 };

	if (InNewState == ECheckBoxState::Checked)
	{
		value.NewValue = 1;
		SetValues<int>(1, &value);
	}
	else
	{
		value.NewValue = 0;
		SetValues<int>(1, &value);
	}
}

ECheckBoxState SSubstanceEditorPanel::GetToggleValue(SubstanceAir::InputInstanceBase* Input) const
{
	ECheckBoxState State;
	(GetInputValue<int32>(Input, 0).GetValue() > 0.0f) ? State = ECheckBoxState::Checked : State = ECheckBoxState::Unchecked;
	return State;
}

void SSubstanceEditorPanel::UpdateColor(FLinearColor NewColor, SubstanceAir::InputInstanceBase* Input)
{
	if (Substance_IType_Float == Input->mDesc.mType)
	{
		FInputValue<float> value = { NewColor.Desaturate(1.0f).R, Input, 0 };
		SetValues<float>(1, &value);
		return;
	}

	FInputValue<float> values[] =
	{
		{ NewColor.R, Input, 0 },
		{ NewColor.G, Input, 1 },
		{ NewColor.B, Input, 2 },
		{ NewColor.A, Input, 3 },
	};

	if (Substance_IType_Float4 == Input->mDesc.mType)
	{
		SetValues<float>(4, values);
	}
	else
	{
		SetValues<float>(3, values);
	}
}

void SSubstanceEditorPanel::CancelColor(FLinearColor OldColor, SubstanceAir::InputInstanceBase* Input)
{
	UpdateColor(OldColor.HSVToLinearRGB(), Input);
}

FReply SSubstanceEditorPanel::PickColor(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, SubstanceAir::InputInstanceBase* Input)
{
	FColorPickerArgs PickerArgs;

	FLinearColor InputColor = GetColor(Input);

	if (Input->mDesc.mType == Substance_IType_Float4)
	{
		PickerArgs.bUseAlpha = true;
	}
	else
	{
		PickerArgs.bUseAlpha = false;
	}

	PickerArgs.ParentWidget = this->AsShared();
	PickerArgs.bOnlyRefreshOnOk = false;
	PickerArgs.bOnlyRefreshOnMouseUp = false;
	PickerArgs.InitialColorOverride = InputColor;
	PickerArgs.OnColorCommitted = FOnLinearColorValueChanged::CreateSP(this, &SSubstanceEditorPanel::UpdateColor, Input);
	PickerArgs.OnColorPickerCancelled = FOnColorPickerCancelled::CreateSP(this, &SSubstanceEditorPanel::CancelColor, Input);

	OpenColorPicker(PickerArgs);

	return FReply::Handled();
}

FText SSubstanceEditorPanel::GetOutputSizeValue(SubstanceAir::InputInstanceBase* Input, int32 Idx) const
{
	SubstanceAir::InputInstanceInt* TypedInst = (SubstanceAir::InputInstanceInt*)Input;

	int32 SizeValue = FMath::RoundToInt(FMath::Pow(2.0f, (float)TypedInst->getValue()));

	return FText::FromString(FString::Printf(TEXT("%i"), SizeValue));
}

TSharedRef<SWidget> SSubstanceEditorPanel::GetInputWidgetSizePow2(SubstanceAir::InputInstanceBase* Input)
{
	SharedFStringArray* ItemsForWidget = new SharedFStringArray();

	FString CurX = GetOutputSizeValue(Input, 0).ToString();
	FString CurY = GetOutputSizeValue(Input, 1).ToString();

	TSharedPtr< FString > CurrentX;
	TSharedPtr< FString > CurrentY;

	int32 CurrentPow2 = OutputSizePow2Min;
	while (CurrentPow2 <= OutputSizePow2Max)
	{
		int32 SizeValue = FMath::Pow(2.0f, (float)CurrentPow2);
		CurrentPow2++;
		ItemsForWidget->Add(MakeShareable(new FString(FString::Printf(TEXT("%i"), SizeValue))));

		if (CurX == FString(FString::Printf(TEXT("%i"), SizeValue)))
		{
			CurrentX = ItemsForWidget->Last();
		}
		if (CurY == FString(FString::Printf(TEXT("%i"), SizeValue)))
		{
			CurrentY = ItemsForWidget->Last();
		}
	}

	ComboBoxLabels.Add(MakeShareable(ItemsForWidget));

	TSharedPtr< SComboBox< TSharedPtr< FString > > > WidgetSizeX = SNew(SComboBox< TSharedPtr<FString> >)
		.InitiallySelectedItem(CurrentX)
		.OptionsSource(ComboBoxLabels.Last().Get())
		.OnGenerateWidget(this, &SSubstanceEditorPanel::MakeInputSizeComboWidget)
		.OnSelectionChanged(this, &SSubstanceEditorPanel::OnSizeComboboxSelectionChanged, Input, 0)
		.ContentPadding(0)
		.Content()
		[
			SNew(STextBlock)
			.Text(this, &SSubstanceEditorPanel::GetOutputSizeValue, Input, 0)
		];

	TSharedPtr< SComboBox< TSharedPtr< FString > > > WidgetSizeY = SNew(SComboBox< TSharedPtr<FString> >)
		.InitiallySelectedItem(CurrentY)
		.OptionsSource(ComboBoxLabels.Last().Get())
		.OnGenerateWidget(this, &SSubstanceEditorPanel::MakeInputSizeComboWidget)
		.OnSelectionChanged(this, &SSubstanceEditorPanel::OnSizeComboboxSelectionChanged, Input, 1)
		.ContentPadding(0)
		.Content()
		[
			SNew(STextBlock)
			.Text(this, &SSubstanceEditorPanel::GetOutputSizeValue, Input, 1)
		];

	RatioLocked.Set(true);

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(0.3f)
		[
			GetBaseInputWidget(Input, TEXT("Output Size"))
		]
	+ SHorizontalBox::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		[
			WidgetSizeX.ToSharedRef()
		]
	+ SHorizontalBox::Slot()
		[
			WidgetSizeY.ToSharedRef()
		]
	+ SHorizontalBox::Slot()
		[
			SNew(SCheckBox)
			.OnCheckStateChanged(this, &SSubstanceEditorPanel::OnLockRatioValueChanged)
		.IsChecked(this, &SSubstanceEditorPanel::GetLockRatioValue)
		.Content()
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Lock Ratio")))
		]
		]
		];
}

void SSubstanceEditorPanel::OnLockRatioValueChanged(ECheckBoxState InNewState)
{
	RatioLocked.Set(InNewState == ECheckBoxState::Checked ? true : false);
}

ECheckBoxState SSubstanceEditorPanel::GetLockRatioValue() const
{
	return RatioLocked.Get() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

TSharedRef<SWidget> SSubstanceEditorPanel::MakeInputSizeComboWidget(TSharedPtr<FString> InItem)
{
	return SNew(STextBlock).Text(FText::FromString(*InItem));
}

void SSubstanceEditorPanel::OnSizeComboboxSelectionChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo, SubstanceAir::InputInstanceBase* Input, int Idx)
{
	if (!Item.IsValid())
	{
		return;
	}

	SubstanceAir::InputInstanceInt2* TypedInst = (SubstanceAir::InputInstanceInt2*)Input;

	float PrevValue = (float)TypedInst->getValue()[Idx];; // Values[Idx];

	int SizeValue = FCString::Atoi(**Item);
	float NewValue = FMath::Log2((float)SizeValue);

	FInputValue<int32> values[2];

	values[0].NewValue = NewValue;
	values[0].Input = Input;
	values[0].Index = Idx;

	FScopedTransaction Transaction(TEXT("SubstanceEd"), NSLOCTEXT("SubstanceEditor", "SubstanceSetOutputSize", "Substance change output size"), Graph);
	Graph->Modify();

	if (RatioLocked.Get())
	{
		const float DeltaValue = NewValue - PrevValue;
		const int OtherIdx = Idx == 0 ? 1 : 0;

		values[1].NewValue = FMath::Clamp((float)TypedInst->getValue()[OtherIdx] + DeltaValue, (float)OutputSizePow2Min, (float)OutputSizePow2Max);;
		values[1].Input = Input;
		values[1].Index = OtherIdx;

		SetValues<int>(2, values);
	}
	else
	{
		SetValues<int>(1, values);
	}
}

TSharedRef<SWidget> SSubstanceEditorPanel::GetInputWidgetRandomSeed(SubstanceAir::InputInstanceBase* Input)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(0.3f)
		[
			GetBaseInputWidget(Input, TEXT("Random Seed"))
		]
	+ SHorizontalBox::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("Randomize Seed")))
		.OnClicked(this, &SSubstanceEditorPanel::RandomizeSeed, Input)
		]
	+ SHorizontalBox::Slot()
		[
			GetInputWidgetSlider_internal<int32>(Input, 0)
		]
		];
}

FReply SSubstanceEditorPanel::RandomizeSeed(SubstanceAir::InputInstanceBase* Input)
{
	const int32 rand = FMath::Rand();
	FScopedTransaction Transaction(NSLOCTEXT("SubstanceEditor", "SubstanceSetInput", "Substance set input"));
	Graph->Modify();

	FInputValue<int32> value = { rand, Input, 0 };
	SetValues<int>(1, &value);

	return FReply::Handled();
}

void SSubstanceEditorPanel::OnGetClassesForAssetPicker(TArray<const UClass*>& OutClasses)
{
	OutClasses.AddUnique(USubstanceImageInput::StaticClass());

	// disable substance output as input feature for now
	//OutClasses.AddUnique(USubstanceTexture2D::StaticClass());
}

void SSubstanceEditorPanel::OnAssetSelected(const FAssetData& AssetData, SubstanceAir::InputInstanceBase* Input, TSharedPtr<FAssetThumbnail> Thumbnail)
{
	OnSetImageInput(AssetData.GetAsset(), Input, Thumbnail);
}

void SSubstanceEditorPanel::OnUseSelectedImageInput(SubstanceAir::InputInstanceBase* Input, TSharedPtr<FAssetThumbnail> Thumbnail)
{
	// Load selected assets
	FEditorDelegates::LoadSelectedAssetsIfNeeded.Broadcast();

	USelection* Selection = GEditor->GetSelectedObjects();
	if (Selection)
	{
		USubstanceImageInput* ImageInput = Selection->GetTop<USubstanceImageInput>();
		if (ImageInput)
		{
			OnSetImageInput(ImageInput, Input, Thumbnail);
		}
	}
}

TSharedRef<SWidget> SSubstanceEditorPanel::GetImageInputWidget(SubstanceAir::InputInstanceBase* Input)
{
	const int ThumbSize = 64;

	SubstanceAir::InputDescBase* ImgInputDesc = (SubstanceAir::InputDescBase*)&Input->mDesc;
	SubstanceAir::InputInstanceImage* ImageInput = (SubstanceAir::InputInstanceImage*)Input;

	TSharedPtr<FAssetThumbnail> Thumbnail;
	if (ImageInput->getImage() && ImageInput->getImage()->mUserData != 0)
	{
		Thumbnail = MakeShareable(new FAssetThumbnail(reinterpret_cast<InputImageData*>(ImageInput->getImage()->mUserData)->ImageUObjectSource,
			ThumbSize, ThumbSize, ThumbnailPool));

		ThumbnailInputs.Add(Thumbnail, ImageInput);
	}
	else
	{
		Thumbnail = MakeShareable(new FAssetThumbnail(NULL, ThumbSize, ThumbSize, ThumbnailPool));
		ThumbnailInputs.Add(Thumbnail, ImageInput);
	}

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(0.3f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		[
			SNew(STextBlock)
			.Text(FText::FromString(Input->mDesc.mLabel.c_str()))
		.Font(FEditorStyle::GetFontStyle("BoldFont"))
		]
	+ SHorizontalBox::Slot()
		[
			SNew(STextBlock)
			.Text(FText::FromString(ImgInputDesc->mGuiDescription.c_str()))
		]
	+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SSpacer)
		]
	+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		[
			SNew(SAssetDropTarget)
			.OnIsAssetAcceptableForDrop(this, &SSubstanceEditorPanel::OnAssetDraggedOver)
		.OnAssetDropped(this, &SSubstanceEditorPanel::OnAssetDropped, Input, Thumbnail)
		[
			SNew(SBox)
			.WidthOverride(FOptionalSize(ThumbSize))
		.HeightOverride(FOptionalSize(ThumbSize))
		[
			Thumbnail->MakeThumbnailWidget()
		]
		]
		]
		]

		]
	+ SHorizontalBox::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(2.0f, 1.0f)
		[
			PropertyCustomizationHelpers::MakeAssetPickerAnchorButton(
				FOnGetAllowedClasses::CreateSP(this, &SSubstanceEditorPanel::OnGetClassesForAssetPicker),
				FOnAssetSelected::CreateSP(this, &SSubstanceEditorPanel::OnAssetSelected, Input, Thumbnail))
		]
	+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(2.0f, 1.0f)
		[
			PropertyCustomizationHelpers::MakeUseSelectedButton(FSimpleDelegate::CreateSP(this, &SSubstanceEditorPanel::OnUseSelectedImageInput, Input, Thumbnail))
		]
	+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(2.0f, 1.0f)
		[
			PropertyCustomizationHelpers::MakeBrowseButton(FSimpleDelegate::CreateSP(this, &SSubstanceEditorPanel::OnBrowseImageInput, ImageInput))
		]
	+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(2.0f, 1.0f)
		[
			SNew(SButton)
			.ToolTipText(NSLOCTEXT("PropertyEditor", "ResetToDefaultToolTip", "Reset to Default"))
		.ButtonStyle(FEditorStyle::Get(), "NoBorder")
		.OnClicked(this, &SSubstanceEditorPanel::OnResetImageInput, Input, Thumbnail)
		.Content()
		[
			SNew(SImage)
			.Image(FEditorStyle::GetBrush("PropertyWindow.DiffersFromDefault"))
		]
		]
	+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SSpacer)
		]
		];
}

FString SSubstanceEditorPanel::GetImageInputPath(SubstanceAir::InputInstanceBase* Input)
{
	SubstanceAir::InputInstanceImage* TypedInst = (SubstanceAir::InputInstanceImage*)Input;
	FString PathName;

	if (TypedInst->getImage())
	{
		PathName = reinterpret_cast<InputImageData*>(TypedInst->getImage()->mUserData)->ImageUObjectSource->GetPathName();
	}

	return PathName;
}

FReply SSubstanceEditorPanel::OnResetImageInput(SubstanceAir::InputInstanceBase* Input, TSharedPtr<FAssetThumbnail> Thumbnail)
{
	OnSetImageInput(NULL, Input, Thumbnail);
	return FReply::Handled();
}

FReply SSubstanceEditorPanel::OnResetInput(SubstanceAir::InputInstanceBase* Input)
{
	FScopedTransaction Transaction(TEXT("SubstanceEd"), NSLOCTEXT("SubstanceEditor", "SubstanceResetInput", "Substance reset input"), Graph);
	Graph->Modify();

	Substance::Helpers::ResetToDefault(Input);

	for (int32 i = 0; i < Graph->Instance->getOutputs().size(); ++i)
	{
		Graph->Instance->getOutputs()[i]->flagAsDirty();
	}

	TArray<SubstanceAir::GraphInstance*> Graphs;
	Graphs.AddUnique(Graph->Instance);
	Substance::Helpers::RenderAsync(Graphs);

	return FReply::Handled();
}

void SSubstanceEditorPanel::OnSetImageInput(const UObject* InObject, SubstanceAir::InputInstanceBase* Input, TSharedPtr<FAssetThumbnail> Thumbnail)
{
	FScopedTransaction Transaction(TEXT("SubstanceEd"),
		InObject ?
		NSLOCTEXT("SubstanceEditor", "SubstanceSetInput", "Substance set image input") :
		NSLOCTEXT("SubstanceEditor", "SubstanceSetInput", "Substance reset image input"),
		Graph);
	Graph->Modify();

	SubstanceAir::InputInstanceImage* InputInst = (SubstanceAir::InputInstanceImage*)Input;

	UObject* NewInput = Cast<UObject>(const_cast<UObject*>(InObject));

	Substance::Helpers::UpdateInput(Graph->Instance, (uint32)InputInst->mDesc.mUid, NewInput);

	Substance::Helpers::RenderAsync(Graph->Instance);

	Thumbnail->SetAsset(InObject);
	ThumbnailPool->Tick(0.1f);
}

TSharedRef<SWidget> SSubstanceEditorPanel::GetInputWidgetAngle(SubstanceAir::InputInstanceBase* Input)
{
	return GetInputWidgetSlider<float>(Input);
}

TSharedRef<SWidget> SSubstanceEditorPanel::GetInputWidget(SubstanceAir::InputInstanceBase* Input)
{
	// special widget for random seed
	if (Input->mDesc.mIdentifier == "$randomseed")
	{
		return GetInputWidgetRandomSeed(Input);
	}
	if (Input->mDesc.mIdentifier == "$outputsize")
	{
		return GetInputWidgetSizePow2(Input);
	}

	switch (Input->mDesc.mGuiWidget)
	{
	default:
	case SubstanceAir::Input_NoWidget:
	case SubstanceAir::Input_Slider:
	case SubstanceAir::Input_Color:

		switch (Input->mDesc.mType)
		{
		case SubstanceInputType::Substance_IType_Float:
		case SubstanceInputType::Substance_IType_Float2:
		case SubstanceInputType::Substance_IType_Float3:
		case SubstanceInputType::Substance_IType_Float4:
			return GetInputWidgetSlider< float >(Input);

		default:
		case SubstanceInputType::Substance_IType_Integer:
		case SubstanceInputType::Substance_IType_Integer2:
		case SubstanceInputType::Substance_IType_Integer3:
		case SubstanceInputType::Substance_IType_Integer4:
			return GetInputWidgetSlider< int32 >(Input);
		}

	case SubstanceAir::Input_Angle:
		return GetInputWidgetAngle(Input);

	case SubstanceAir::Input_Combobox:
		return GetInputWidgetCombobox(Input);

	case SubstanceAir::Input_Togglebutton:
		return GetInputWidgetTogglebutton(Input);
	}
}

template< typename T > TSharedRef<SHorizontalBox> SSubstanceEditorPanel::GetInputWidgetSlider(SubstanceAir::InputInstanceBase* Input)
{
	check(Input);
	check(Input->mDesc.mType != Substance_IType_Image);

	int32 SliderCount = 1;

	switch (Input->mDesc.mType)
	{
	case Substance_IType_Integer2:
	case Substance_IType_Float2:
		SliderCount = 2;
		break;
	case Substance_IType_Integer3:
	case Substance_IType_Float3:
		SliderCount = 3;
		break;
	case Substance_IType_Integer4:
	case Substance_IType_Float4:
		SliderCount = 4;
		break;
	}

	TSharedPtr<SHorizontalBox> InputContainer = SNew(SHorizontalBox);

	TSharedPtr<SVerticalBox> SlidersBox = SNew(SVerticalBox);
	for (int32 i = 0; i < SliderCount; ++i)
	{
		SlidersBox->AddSlot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
			.Padding(0.0f, 1.0f)
			[
				GetInputWidgetSlider_internal<T>(Input, i)
			]
			];
	}

	InputContainer->AddSlot()
		[
			SlidersBox.ToSharedRef()
		];

	if (Input->mDesc.mGuiWidget == SubstanceAir::Input_Color)
	{
		InputContainer->AddSlot()
			.FillWidth(0.1f)
			[
				SNew(SColorBlock)
				.Color(this, &SSubstanceEditorPanel::GetColor, Input)
			.ColorIsHSV(false)
			.ShowBackgroundForAlpha(true)
			.ToolTipText(FText::FromString(TEXT("Pick Color")))
			.OnMouseButtonDown(this, &SSubstanceEditorPanel::PickColor, Input)
			.UseSRGB(false)
			];
	}

	TSharedPtr<SHorizontalBox> WidgetsBox =
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(0.3f)
		[
			GetBaseInputWidget(Input)
		]
	+ SHorizontalBox::Slot()
		[
			InputContainer.ToSharedRef()
		];

	return WidgetsBox.ToSharedRef();
}

FLinearColor SSubstanceEditorPanel::GetColor(SubstanceAir::InputInstanceBase* Input) const
{
	SubstanceAir::InputInstanceFloat4* InputInst = (SubstanceAir::InputInstanceFloat4*)Input;

	TArray<float> Values;
	Substance::GetNumericalInputValue((SubstanceAir::InputInstanceNumericalBase*)Input, Values);

	FLinearColor InputColor;
	if (Values.Num() == 1)
	{
		InputColor = FLinearColor(Values[0], Values[0], Values[0], 1.0f);
	}
	else if (Values.Num() == 3)
	{
		InputColor = FLinearColor(Values[0], Values[1], Values[2], 1.0f);
	}
	else if (Values.Num() == 4)
	{
		InputColor = FLinearColor(Values[0], Values[1], Values[2], Values[3]);
	}

	return InputColor;
}

void SSubstanceEditorPanel::BeginSliderMovement(SubstanceAir::InputInstanceBase* Input)
{
	FScopedTransaction Transaction(NSLOCTEXT("SubstanceEditor", "SubstanceSetInput", "Substance set input"));
	Graph->Modify();
}

template< typename T, typename U> const U GetMinComponent(const SubstanceAir::InputDescBase* Desc, int Idx, bool& Clamped)
{
	const SubstanceAir::InputDescNumerical<T>* TypedDesc = (SubstanceAir::InputDescNumerical<T>*)Desc;
	const U MinValue = TypedDesc->mMinValue[Idx];
	return MinValue;
}

template< typename T, typename U> const U GetMaxComponent(const SubstanceAir::InputDescBase* Desc, int Idx, bool& Clamped)
{
	SubstanceAir::InputDescNumerical<T>* TypedDesc = (SubstanceAir::InputDescNumerical<T>*)Desc;
	const U MaxValue = TypedDesc->mMaxValue[Idx];
	return MaxValue;
}

template< typename T> void GetMinMaxValues(const SubstanceAir::InputDescBase* Desc, const int32 i, T& Min, T& Max, bool& Clamped)
{
	switch (Desc->mType)
	{
	case Substance_IType_Integer:
	{
		SubstanceAir::InputDescNumerical<int32>* TypedDesc = (SubstanceAir::InputDescNumerical<int32>*)Desc;
		Min = TypedDesc->mMinValue;
		Max = TypedDesc->mMaxValue;
	}
	break;
	case Substance_IType_Float:
	{
		SubstanceAir::InputDescNumerical<float>* TypedDesc = (SubstanceAir::InputDescNumerical<float>*)Desc;
		Min = TypedDesc->mMinValue;
		Max = TypedDesc->mMaxValue;
	}
	break;
	case Substance_IType_Integer2:
	{
		Min = GetMinComponent<SubstanceAir::Vec2Int, int>(Desc, i, Clamped);
		Max = GetMaxComponent<SubstanceAir::Vec2Int, int>(Desc, i, Clamped);
	}
	break;
	case Substance_IType_Float2:
	{
		Min = GetMinComponent<SubstanceAir::Vec2Float, float>(Desc, i, Clamped);
		Max = GetMaxComponent<SubstanceAir::Vec2Float, float>(Desc, i, Clamped);
	}
	break;
	case Substance_IType_Integer3:
	{
		Min = GetMinComponent<SubstanceAir::Vec3Int, int>(Desc, i, Clamped);
		Max = GetMaxComponent<SubstanceAir::Vec3Int, int>(Desc, i, Clamped);
	}
	break;
	case Substance_IType_Float3:
	{
		Min = GetMinComponent<SubstanceAir::Vec3Float, float>(Desc, i, Clamped);
		Max = GetMaxComponent<SubstanceAir::Vec3Float, float>(Desc, i, Clamped);
	}
	break;
	case Substance_IType_Integer4:
	{
		Min = GetMinComponent<SubstanceAir::Vec4Int, int>(Desc, i, Clamped);
		Max = GetMaxComponent<SubstanceAir::Vec4Int, int>(Desc, i, Clamped);
	}
	break;
	case Substance_IType_Float4:
	{
		Min = GetMinComponent<SubstanceAir::Vec4Float, float>(Desc, i, Clamped);
		Max = GetMaxComponent<SubstanceAir::Vec4Float, float>(Desc, i, Clamped);
	}
	break;
	}
}

template< > TSharedRef< SNumericEntryBox< float > > SSubstanceEditorPanel::GetInputWidgetSlider_internal<float>(SubstanceAir::InputInstanceBase* Input, const int32 SliderIndex)
{
	float delta = 0;
	float SliderMin = 0;
	float SliderMax = 0;
	bool Clamped = false;

	GetMinMaxValues(&Input->mDesc, SliderIndex, SliderMin, SliderMax, Clamped);

	float MinValue = SliderMin;
	float MaxValue = SliderMax;

	if (Clamped == false)
	{
		if (Input->mDesc.mGuiWidget == SubstanceAir::Input_Color)
		{
			MinValue = 0.0f;
			MaxValue = 1.0f;
		}
		else
		{
			MinValue = TNumericLimits<float>::Lowest();
			MaxValue = TNumericLimits<float>::Max();
		}
	}

	if (SliderMin == SliderMax || SliderMin > SliderMax)
	{
		if (Input->mDesc.mGuiWidget == SubstanceAir::Input_Color)
		{
			SliderMin = 0.0f;
			SliderMax = 1.0f;
		}
		else
		{
			SliderMin = TNumericLimits<float>::Lowest();
			SliderMax = TNumericLimits<float>::Max();
		}
	}

	return SNew(SNumericEntryBox<float>)
		.Value(this, &SSubstanceEditorPanel::GetInputValue<float>, Input, SliderIndex)
		.OnValueChanged(this, &SSubstanceEditorPanel::SetValue<float>, Input, SliderIndex)
		.OnBeginSliderMovement(this, &SSubstanceEditorPanel::BeginSliderMovement, Input)
		.ToolTipText(FText::FromString(Input->mDesc.mIdentifier.c_str()))
		.AllowSpin(true)
		.Delta(0.001f)
		.LabelPadding(FMargin(0.0f, 1.1f))
		.MinValue(MinValue)
		.MaxValue(MaxValue)
		.MinSliderValue(SliderMin)
		.MaxSliderValue(SliderMax);
}

template< > TSharedRef< SNumericEntryBox< int32 > > SSubstanceEditorPanel::GetInputWidgetSlider_internal<int32>(SubstanceAir::InputInstanceBase* Input, const int32 SliderIndex)
{
	float delta = 0;
	int32 SliderMin = 0;
	int32 SliderMax = 0;
	bool Clamped = false;

	GetMinMaxValues(&Input->mDesc, SliderIndex, SliderMin, SliderMax, Clamped);

	int32 MinValue = SliderMin;
	int32 MaxValue = SliderMax;

	if (Clamped == false)
	{
		MinValue = TNumericLimits<int32>::Lowest();
		MaxValue = TNumericLimits<int32>::Max();
	}

	if (SliderMin == SliderMax || SliderMin > SliderMax)
	{
		SliderMin = TNumericLimits<int32>::Lowest();
		SliderMax = TNumericLimits<int32>::Max();
	}

	return SNew(SNumericEntryBox<int32>)
		.Value(this, &SSubstanceEditorPanel::GetInputValue<int32>, Input, SliderIndex)
		.OnValueChanged(this, &SSubstanceEditorPanel::SetValue<int>, Input, SliderIndex)
		.OnBeginSliderMovement(this, &SSubstanceEditorPanel::BeginSliderMovement, Input)
		.ToolTipText(FText::FromString(Input->mDesc.mIdentifier.c_str()))
		.AllowSpin(true)
		.Delta(0)
		.LabelPadding(FMargin(0.0f, 1.1f))
		.MinValue(MinValue)
		.MaxValue(MaxValue)
		.MinSliderValue(SliderMin)
		.MaxSliderValue(SliderMax);
}

#undef LOC_NAMESPACE