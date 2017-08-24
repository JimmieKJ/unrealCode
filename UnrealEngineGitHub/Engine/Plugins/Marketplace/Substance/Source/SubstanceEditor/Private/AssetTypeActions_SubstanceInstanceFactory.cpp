// Copyright 2016 Allegorithmic Inc. All rights reserved.
// File: AssetTypeActions_SubstanceInstanceFactory.cpp

#include "SubstanceEditorPrivatePCH.h"
#include "SubstanceCoreHelpers.h"
#include "SubstanceCoreClasses.h"
#include "SubstanceInstanceFactory.h"
#include "SubstanceFactory.h"

#include "substance/framework/graph.h"
#include "substance/framework/package.h"

#include "ContentBrowserModule.h"
#include "AssetTypeActions_Base.h"
#include "AssetTypeActions_SubstanceInstanceFactory.h"
#include "AssetRegistryModule.h"
#include "ReferencedAssetsUtils.h"
#include "Text.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

void FAssetTypeActions_SubstanceInstanceFactory::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder)
{
	auto InstanceFactory = GetTypedWeakObjectPtrs<USubstanceInstanceFactory>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("SubstanceInstanceFactory_Edit", "Edit"),
		LOCTEXT("SubstanceInstanceFactory_EditTooltip", "Opens the selected graph instances in the graph instance editor."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &FAssetTypeActions_SubstanceInstanceFactory::ExecuteEdit, InstanceFactory),
			FCanExecuteAction()
		)
	);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("SubstanceInstanceFactory_Reimport", "Reimport"),
		LOCTEXT("SubstanceInstanceFactory_ReimportTooltip", "Reimports the selected graph instances."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &FAssetTypeActions_SubstanceInstanceFactory::ExecuteReimport, InstanceFactory),
			FCanExecuteAction()
		)
	);

	// show list of graph contained in instance factory when a single one is selected
	if (InObjects.Num() == 1)
	{
		MenuBuilder.AddSubMenu(
			LOCTEXT("SubstanceInstanceFactory_CreateInstance", "Create Graph Instance"),
			LOCTEXT("SubstanceInstanceFactory_CreateInstance", "Create an instance of the specified graph."),
			FNewMenuDelegate::CreateRaw(this, &FAssetTypeActions_SubstanceInstanceFactory::CreateInstanceSubMenu, InstanceFactory)
		);
	}

	MenuBuilder.AddMenuEntry(
		LOCTEXT("SubstanceInstanceFactory_DeleteWithOutputs", "Delete with instances and outputs"),
		LOCTEXT("SubstanceInstanceFactory_DeleteWithOutputs", "Delete this instanceFactory with all its instances and output textures."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &FAssetTypeActions_SubstanceInstanceFactory::ExecuteDeleteWithOutputs, InstanceFactory),
			FCanExecuteAction()
		)
	);
}

void FAssetTypeActions_SubstanceInstanceFactory::CreateInstanceSubMenu(class FMenuBuilder& MenuBuilder, TArray<TWeakObjectPtr<USubstanceInstanceFactory>> InstanceFactory)
{
	// show list of graph contained in instance factory when a single one is selected
	auto ItGraphDesc(InstanceFactory[0]->SubstancePackage->getGraphs().begin());
	for (; ItGraphDesc != InstanceFactory[0]->SubstancePackage->getGraphs().end(); ++ItGraphDesc)
	{
		const SubstanceAir::GraphDesc* GraphDescItr = &(*ItGraphDesc);
		FString CurrentGraphLabel(GraphDescItr->mLabel.c_str());
		FText text = FText::FromString(FString::Printf(
			*NSLOCTEXT("SubstanceInstanceFactory", "New Instance of ", "%s").ToString(), *CurrentGraphLabel));

		FString CurrentName = InstanceFactory[0]->GetName();
		MenuBuilder.AddMenuEntry(
			text,
			FText::FromString(TEXT("")),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &FAssetTypeActions_SubstanceInstanceFactory::ExecuteInstantiate, GraphDescItr, InstanceFactory[0].Get()),
				FCanExecuteAction()
			));
	}
}

void FAssetTypeActions_SubstanceInstanceFactory::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
}

void FAssetTypeActions_SubstanceInstanceFactory::ExecuteEdit(TArray<TWeakObjectPtr<USubstanceInstanceFactory>> Objects)
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

void FAssetTypeActions_SubstanceInstanceFactory::ExecuteReimport(TArray<TWeakObjectPtr<USubstanceInstanceFactory>> Objects)
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if (Object)
		{
			FReimportManager::Instance()->Reimport(Object, true, true);
		}
	}
}

void FAssetTypeActions_SubstanceInstanceFactory::ExecuteInstantiate(const SubstanceAir::GraphDesc* Desc, USubstanceInstanceFactory* Factory)
{
	bool bOperationCanceled = false;
	Substance::FSubstanceImportOptions ImportOptions;
	ImportOptions.bForceCreateInstance = true;

	Substance::GetImportOptions(
		FString(Desc->mLabel.c_str()),
		Factory->GetOuter()->GetName(),
		ImportOptions,
		bOperationCanceled);

	if (bOperationCanceled)
	{
		return;
	}

	UObject* InstanceOuter = CreatePackage(NULL, *ImportOptions.InstanceDestinationPath);

	USubstanceGraphInstance* Instance =
		Substance::Helpers::InstantiateGraph(
			Factory,
			*Desc,
			InstanceOuter,
			ImportOptions.InstanceName,
			true // bCreateOutputs
		);

	Substance::Helpers::RenderSync(Instance->Instance);

	{
		TArray<SubstanceAir::GraphInstance*> Instances;
		Instances.AddUnique(Instance->Instance);
		Substance::Helpers::UpdateTextures(Instances);
	}

	if (ImportOptions.bCreateMaterial)
	{
		UObject* MaterialParent = CreatePackage(NULL, *ImportOptions.MaterialDestinationPath);
		Substance::Helpers::CreateMaterial(Instance, ImportOptions.MaterialName, MaterialParent);
	}

	TArray<UObject*> AssetList;
	AssetList.AddUnique(Instance);
	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowserModule.Get().SyncBrowserToAssets(AssetList, true);
}

void FAssetTypeActions_SubstanceInstanceFactory::ExecuteDeleteWithOutputs(TArray<TWeakObjectPtr<USubstanceInstanceFactory>> Objects)
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		USubstanceInstanceFactory* InstanceFactory = (USubstanceInstanceFactory *)((*ObjIt).Get());
		if (InstanceFactory)
		{
			Substance::Helpers::RegisterForDeletion(InstanceFactory);
		}
	}
}

#undef LOC_NAMESPACE