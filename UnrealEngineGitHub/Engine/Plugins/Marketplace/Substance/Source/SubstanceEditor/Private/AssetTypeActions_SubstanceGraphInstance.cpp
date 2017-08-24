// Copyright 2016 Allegorithmic Inc. All rights reserved.
// File: AssetTypeActions_SubstanceGraphInstance.h

#include "SubstanceEditorPrivatePCH.h"
#include "AssetTypeActions_Base.h"
#include "AssetTypeActions_SubstanceGraphInstance.h"

#include "SubstanceCoreClasses.h"
#include "SubstanceCoreHelpers.h"
#include "SubstanceGraphInstance.h"
#include "SubstanceEditorModule.h"

#include "substance/framework/preset.h"
#include "substance/framework/graph.h"

#include "ContentBrowserModule.h"
#include "AssetRegistryModule.h"
#include "ReferencedAssetsUtils.h"
#include "SNotificationList.h"
#include "NotificationManager.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

void FAssetTypeActions_SubstanceGraphInstance::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder)
{
	auto Graphs = GetTypedWeakObjectPtrs<USubstanceGraphInstance>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("SubstanceGraphInstance_Edit", "Edit"),
		LOCTEXT("SubstanceGraphInstance_EditTooltip", "Opens the selected graph instances in the graph instance editor."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &FAssetTypeActions_SubstanceGraphInstance::ExecuteEdit, Graphs),
			FCanExecuteAction()
		)
	);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("SubstanceGraphInstance_Reimport", "Reimport"),
		LOCTEXT("SubstanceGraphInstance_ReimportTooltip", "Reimports the selected graph instances."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &FAssetTypeActions_SubstanceGraphInstance::ExecuteReimport, Graphs),
			FCanExecuteAction()
		)
	);

	MenuBuilder.AddSubMenu(
		LOCTEXT("SubstanceGraphInstance_Preset", "Presets"),
		LOCTEXT("SubstanceGraphInstance_PresetsTooltip", "Deal with presets."),
		FNewMenuDelegate::CreateRaw(this, &FAssetTypeActions_SubstanceGraphInstance::CreatePresetsSubMenu, Graphs)
	);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("SubstanceGraphInstance_FindSubstanceInstanceFactory", "Find Instance Factory"),
		LOCTEXT("SubstanceGraphInstance_FindSubstanceInstanceFactory", "Find parent instance Factory in content browser."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &FAssetTypeActions_SubstanceGraphInstance::ExecuteFindParentFactory, Graphs),
			FCanExecuteAction()
		)
	);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("SubstanceGraphInstance_DeleteWithOutputs", "Delete with outputs"),
		LOCTEXT("SubstanceGraphInstance_DeleteWithOutputs", "Delete this instance with all its output textures."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &FAssetTypeActions_SubstanceGraphInstance::ExecuteDeleteWithOutputs, Graphs),
			FCanExecuteAction()
		)
	);
}

void FAssetTypeActions_SubstanceGraphInstance::CreatePresetsSubMenu(class FMenuBuilder& MenuBuilder, TArray<TWeakObjectPtr<USubstanceGraphInstance>> Graphs)
{
	MenuBuilder.AddMenuEntry(
		LOCTEXT("SubstanceGraphInstance_Import_Presets", "Import Presets"),
		LOCTEXT("SubstanceGraphInstance_Import_presetsTooltip", "Import presets"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &FAssetTypeActions_SubstanceGraphInstance::ExecuteImportPresets, Graphs),
			FCanExecuteAction()
		)
	);
	MenuBuilder.AddMenuEntry(
		LOCTEXT("SubstanceGraphInstance_Export_Presets", "Export Presets"),
		LOCTEXT("SubstanceGraphInstance_Export_presetsTooltip", "Export presets"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &FAssetTypeActions_SubstanceGraphInstance::ExecuteExportPresets, Graphs),
			FCanExecuteAction()
		)
	);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("SubstanceGraphInstance_ResetInstance", "Reset default values"),
		LOCTEXT("SubstanceGraphInstance_ResetInstanceTooltip", "Reset Inputs to default values"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &FAssetTypeActions_SubstanceGraphInstance::ExecuteResetDefault, Graphs),
			FCanExecuteAction()
		)
	);
}

void FAssetTypeActions_SubstanceGraphInstance::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Graph = Cast<USubstanceGraphInstance>(*ObjIt);
		if (Graph != NULL && Graph->Instance)
		{
			ISubstanceEditorModule* SubstanceEditorModule = &FModuleManager::LoadModuleChecked<ISubstanceEditorModule>("SubstanceEditor");
			SubstanceEditorModule->CreateSubstanceEditor(EditWithinLevelEditor, Graph);
		}
		else
		{
			FText NotificationText;
			FFormatNamedArguments Args;
			Args.Add(TEXT("ObjectName"), FText::FromString(Graph->GetName()));
			NotificationText = FText::Format(FText::FromString(TEXT("{ObjectName} is missing Instance Factory, edit disabled. Please delete object.")), Args);

			FNotificationInfo Info(NotificationText);
			Info.ExpireDuration = 5.0f;
			Info.bUseLargeFont = false;
			TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
			if (Notification.IsValid())
			{
				Notification->SetCompletionState(SNotificationItem::CS_Fail);
			}
		}
	}
}

void FAssetTypeActions_SubstanceGraphInstance::ExecuteEdit(TArray<TWeakObjectPtr<USubstanceGraphInstance>> Objects)
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if (Object)
		{
			FAssetEditorManager::Get().OpenEditorForAsset(Object);
		}
	}
}

void FAssetTypeActions_SubstanceGraphInstance::ExecuteReimport(TArray<TWeakObjectPtr<USubstanceGraphInstance>> Objects)
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if (Object)
		{
			FReimportManager::Instance()->Reimport(Object);
		}
	}
}

void FAssetTypeActions_SubstanceGraphInstance::ExecuteImportPresets(TArray<TWeakObjectPtr<USubstanceGraphInstance>> Objects)
{
	FString PresetFileName;
	TArray<SubstanceAir::Presets> presets;

	std::vector<SubstanceAir::Presets> PresetsVec;
	TArray<SubstanceAir::Presets>::TIterator PreItr(presets);
	for (; PreItr; ++PreItr)
	{
		PresetsVec.push_back(*PreItr);
	}

	TArray<TWeakObjectPtr<USubstanceGraphInstance>>::TIterator ObjIt(Objects);
	for (; ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if (Object)
		{
			PresetFileName = Substance::Helpers::ImportPresetFile();
			FString Data;
			FFileHelper::LoadFileToString(Data, *PresetFileName);

			Substance::Helpers::ParsePresetAndApply(PresetsVec[0], Data, (USubstanceGraphInstance*)Object);

			TArray<SubstanceAir::GraphInstance*> Graphs;
			Graphs.AddUnique(((USubstanceGraphInstance*)Object)->Instance);
			Substance::Helpers::RenderAsync(Graphs);
		}
	}
}

void FAssetTypeActions_SubstanceGraphInstance::ExecuteExportPresets(TArray<TWeakObjectPtr<USubstanceGraphInstance>> Objects)
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if (Object)
		{
			Substance::Helpers::ExportPresetFromGraph((USubstanceGraphInstance*)Object);
		}
	}
}

void FAssetTypeActions_SubstanceGraphInstance::ExecuteResetDefault(TArray<TWeakObjectPtr<USubstanceGraphInstance>> Objects)
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		USubstanceGraphInstance* Graph = (*ObjIt).Get();
		if (Graph)
		{
			Substance::Helpers::ResetToDefault(Graph->Instance);
			Substance::Helpers::RenderAsync(Graph->Instance);
		}
	}
}

void FAssetTypeActions_SubstanceGraphInstance::ExecuteFindParentFactory(TArray<TWeakObjectPtr<USubstanceGraphInstance>> Objects)
{
	// select a single object
	if (Objects.Num() == 1)
	{
		USubstanceGraphInstance * Graph = (USubstanceGraphInstance *)(Objects[0].Get());
		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		TArray<UObject*> ParentContainer;
		ParentContainer.AddUnique(Graph->ParentFactory);
		ContentBrowserModule.Get().SyncBrowserToAssets(ParentContainer);
	}
}

void FAssetTypeActions_SubstanceGraphInstance::ExecuteDeleteWithOutputs(TArray<TWeakObjectPtr<USubstanceGraphInstance>> Objects)
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		USubstanceGraphInstance* Graph = (USubstanceGraphInstance *)((*ObjIt).Get());
		if (Graph)
		{
			// destroy selected graph instance
			Substance::Helpers::RegisterForDeletion(Graph);
		}
	}
}

#undef LOC_NAMESPACE