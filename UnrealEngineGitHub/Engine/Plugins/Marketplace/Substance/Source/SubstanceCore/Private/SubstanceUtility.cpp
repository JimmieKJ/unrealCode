// Copyright 2016 Allegorithmic Inc. All rights reserved.
// File: SubstanceUtility.cpp

#include "SubstanceCorePrivatePCH.h"
#include "SubstanceUtility.h"
#include "SubstanceGraphInstance.h"
#include "SubstanceInstanceFactory.h"
#include "SubstanceTexture2D.h"
#include "SubstanceCoreHelpers.h"
#include "SubstanceSettings.h"

#include "Materials/MaterialExpressionTextureSample.h"

USubstanceUtility::USubstanceUtility(class FObjectInitializer const & PCIP) : Super(PCIP)
{
}

TArray<class USubstanceGraphInstance*> USubstanceUtility::GetSubstances(class UMaterialInterface* MaterialInterface)
{
	TArray<class USubstanceGraphInstance*> Substances;

	if (!MaterialInterface)
	{
		return Substances;
	}

	UMaterial* Material = MaterialInterface->GetMaterial();

	for (int32 ExpressionIndex = Material->Expressions.Num() - 1; ExpressionIndex >= 0; ExpressionIndex--)
	{
		UMaterialExpressionTextureSample* Expression = Cast<UMaterialExpressionTextureSample>(Material->Expressions[ExpressionIndex]);

		if (Expression)
		{
			USubstanceTexture2D* SubstanceTexture = Cast<USubstanceTexture2D>(Expression->Texture);

			if (SubstanceTexture && SubstanceTexture->ParentInstance)
			{
				Substances.AddUnique(SubstanceTexture->ParentInstance);
			}
		}
	}

	return Substances;
}

TArray<class USubstanceTexture2D*> USubstanceUtility::GetSubstanceTextures(class USubstanceGraphInstance* GraphInstance)
{
	TArray<class USubstanceTexture2D*> SubstanceTextures;

	if (!GraphInstance)
	{
		return SubstanceTextures;
	}

	for (uint32 Idx = 0; Idx < GraphInstance->Instance->getOutputs().size(); ++Idx)
	{
		SubstanceAir::OutputInstance* OutputInstance = GraphInstance->Instance->getOutputs()[Idx];

		if (OutputInstance->mEnabled && OutputInstance->mUserData != 0)
		{
			SubstanceTextures.Add(reinterpret_cast<OutputInstanceData*>(OutputInstance->mUserData)->Texture.Get());
		}
	}
	return SubstanceTextures;
}

FString USubstanceUtility::GetGraphName(USubstanceGraphInstance* GraphInstance)
{
	FString GraphInstanceName;
	if (GraphInstance)
		GraphInstanceName = GraphInstance->GetInstanceDesc().Name;
	return GraphInstanceName;
}

FString USubstanceUtility::GetFactoryName(USubstanceGraphInstance* GraphInstance)
{
	FString ParentFactoryName;
	if (GraphInstance && GraphInstance->ParentFactory)
		ParentFactoryName = GraphInstance->ParentFactory->GetFName().ToString();
	return ParentFactoryName;
}

float USubstanceUtility::GetSubstanceLoadingProgress()
{
	return Substance::Helpers::GetSubstanceLoadingProgress();
}

USubstanceGraphInstance* USubstanceUtility::CreateGraphInstance(UObject* WorldContextObject, USubstanceInstanceFactory* Factory, int32 GraphDescIndex, FString InstanceName)
{
	//NOTE:: Materials can only be created in editor currently. Look into alternatives for creating a material on runtime creation to
	//make the setup much more simple for the end user.
	check(WorldContextObject);
	USubstanceGraphInstance* GraphInstance = NULL;

	if (Factory && Factory->SubstancePackage && GraphDescIndex < (int32)Factory->SubstancePackage->getGraphs().size())
	{
		if (Factory->GetGenerationMode() == SGM_Baked)
		{
			UE_LOG(LogSubstanceCore, Warning, TEXT("Cannot create Graph Instance for Instance Factory %s, GenerationMode value not set to OnLoadAsync or OnLoadSync!"),
				*Factory->GetName());
			return GraphInstance;
		}

		//Set package parent
		UObject* Outer = WorldContextObject ? WorldContextObject : GetTransientPackage();

		//Copy of instantiate
		GraphInstance = NewObject<USubstanceGraphInstance>(Outer, *InstanceName, RF_NoFlags);

		//Register factory used to create this
		GraphInstance->ParentFactory = Factory;
		Factory->RegisterGraphInstance(GraphInstance);

		//Get a reference to the graph desc used to create our instance
		const SubstanceAir::GraphDesc& Desc = Factory->SubstancePackage->getGraphs()[GraphDescIndex];

		//Create a new instance
		GraphInstance->Instance = AIR_NEW(SubstanceAir::GraphInstance)(Desc);

		//Set up the user data for framework access
		GraphInstance->mUserData.ParentGraph = GraphInstance;
		GraphInstance->Instance->mUserData = (size_t)&GraphInstance->mUserData;

		//Create textures that will not be saved
		Substance::Helpers::CreateTransientTextures(GraphInstance, WorldContextObject);

		//Initial update
		Substance::Helpers::RenderSync(GraphInstance->Instance);
		TArray<SubstanceAir::GraphInstance*> Instances;
		Instances.AddUnique(GraphInstance->Instance);
		Substance::Helpers::UpdateTextures(Instances);
	}

	return GraphInstance;
}

USubstanceGraphInstance* USubstanceUtility::DuplicateGraphInstance(UObject* WorldContextObject, USubstanceGraphInstance* GraphInstance)
{
	check(WorldContextObject);

	USubstanceGraphInstance* NewGraphInstance = NULL;

	if (GraphInstance && GraphInstance->ParentFactory && GraphInstance->ParentFactory->SubstancePackage)
	{
		int idx = 0;
		const SubstanceAir::vector<SubstanceAir::GraphDesc>& Graphs = GraphInstance->ParentFactory->SubstancePackage->getGraphs();
		for (auto itGraph = Graphs.begin(); itGraph != Graphs.end(); ++itGraph, ++idx)
		{
			if (GraphInstance->Instance->mDesc.mParent->getUid() == itGraph->mParent->getUid())
			{
				break;
			}
		}

		NewGraphInstance = CreateGraphInstance(WorldContextObject, GraphInstance->ParentFactory, idx);
		CopyInputParameters(GraphInstance, NewGraphInstance);
	}

	return NewGraphInstance;
}

//NOTE:: For disable and enable outputs, passing the indices is not clear from a user stand point. There is no way to easily see which output is associated
//with each index. It would be beneficial in the long run to try and find a way pass a different parameter to enable / disable an output. 
void USubstanceUtility::EnableInstanceOutputs(UObject* WorldContextObject, USubstanceGraphInstance* GraphInstance, TArray<int32> OutputIndices)
{
	if (!GraphInstance || !GraphInstance->Instance)
	{
		return;
	}

	TArray<int32>::TIterator IdxIt(OutputIndices);
	for (; IdxIt; ++IdxIt)
	{
		if (*IdxIt >= (int32)GraphInstance->Instance->getOutputs().size())
		{
			UE_LOG(LogSubstanceCore, Warning, TEXT("Invalid Output Index passed for %s, index %d out of range!"), *GraphInstance->GetName(), *IdxIt);
			continue;
		}

		SubstanceAir::OutputInstance* OutputInst = GraphInstance->Instance->getOutputs()[*IdxIt];

		//Skip unsupported texture formats
		if (OutputInst->mUserData == 0)
		{
			Substance::Helpers::CreateSubstanceTexture2D(OutputInst, true, FString(), WorldContextObject ? WorldContextObject : GetTransientPackage());
		}
	}

	//Upload some place holder content in the texture to make the texture usable
	Substance::Helpers::CreatePlaceHolderMips(GraphInstance->Instance);
}

void USubstanceUtility::DisableInstanceOutputs(UObject* WorldContextObject, USubstanceGraphInstance* GraphInstance, TArray<int32> OutputIndices)
{
	if (!GraphInstance || !GraphInstance->Instance)
	{
		return;
	}

	TArray<int32>::TIterator IdxIt(OutputIndices);
	for (; IdxIt; ++IdxIt)
	{
		if (*IdxIt >= (int32)GraphInstance->Instance->getOutputs().size())
		{
			UE_LOG(LogSubstanceCore, Warning, TEXT("Invalid Output Index passed for %s, index %d out of range!"), *GraphInstance->GetName(), *IdxIt);
			continue;
		}

		SubstanceAir::OutputInstance* OutputInst = GraphInstance->Instance->getOutputs()[*IdxIt];
		Substance::Helpers::Disable(OutputInst);
	}
}

void USubstanceUtility::CopyInputParameters(USubstanceGraphInstance* SourceGraphInstance, USubstanceGraphInstance* DestGraphInstance)
{
	if (!SourceGraphInstance || !SourceGraphInstance->Instance ||
		!DestGraphInstance || !DestGraphInstance->Instance)
	{
		return;
	}

	SubstanceAir::Preset CurrentSet;
	CurrentSet.fill(*SourceGraphInstance->Instance);
	CurrentSet.apply(*DestGraphInstance->Instance);
}

void USubstanceUtility::ResetInputParameters(USubstanceGraphInstance* GraphInstance)
{
	if (!GraphInstance || !GraphInstance->Instance)
	{
		return;
	}

	Substance::Helpers::ResetToDefault(GraphInstance->Instance);
}

int SizeToPow2(ESubstanceTextureSize res)
{
	switch (res)
	{
	case ERL_16:
		return 4;
	case ERL_32:
		return 5;
	case ERL_64:
		return 6;
	case ERL_128:
		return 7;
	case ERL_256:
		return 8;
	case ERL_512:
		return 9;
	case ERL_1024:
		return 10;
	case ERL_2048:
		return 11;
	case ERL_4096:
	{
		//Check to make sure that we can support 4k textures
		if (GetDefault<USubstanceSettings>()->SubstanceEngine == ESubstanceEngineType::SET_GPU)
		{
			return 12;
		}
		else
		{
			UE_LOG(LogSubstanceCore, Warning, TEXT("Tried to assign texture size to 4k without using the GPU engine - Falling back to 2k!"));
			return 11;
		}
	}
	default:
		return 8;
	}
}

void USubstanceUtility::SetGraphInstanceOutputSize(USubstanceGraphInstance* GraphInstance, ESubstanceTextureSize Width, ESubstanceTextureSize Height)
{
	if (!GraphInstance || !GraphInstance->Instance || !GraphInstance->CanUpdate())
	{
		return;
	}

	TArray<int32> OutputSize;
	OutputSize.Add(SizeToPow2(Width));
	OutputSize.Add(SizeToPow2(Height));

	GraphInstance->SetInputInt(TEXT("$outputsize"), OutputSize);
}

void USubstanceUtility::SetGraphInstanceOutputSizeInt(USubstanceGraphInstance* GraphInstance, int32 Width, int32 Height)
{
	if (!GraphInstance || !GraphInstance->Instance || !GraphInstance->CanUpdate())
	{
		return;
	}

	Width = FMath::Log2(FMath::Pow(2, (int32)FMath::Log2(Width)));
	Height = FMath::Log2(FMath::Pow(2, (int32)FMath::Log2(Height)));

	TArray<int32> OutputSize;
	OutputSize.Add(Width);
	OutputSize.Add(Height);

	GraphInstance->SetInputInt(TEXT("$outputsize"), OutputSize);
}

void USubstanceUtility::SyncRendering(USubstanceGraphInstance* GraphInstance)
{
	if (!GraphInstance || !GraphInstance->Instance || !GraphInstance->CanUpdate())
	{
		return;
	}

	Substance::Helpers::RenderSync(GraphInstance->Instance);
	TArray<SubstanceAir::GraphInstance*> Instances;
	Instances.AddUnique(GraphInstance->Instance);
	Substance::Helpers::UpdateTextures(Instances);
}

void USubstanceUtility::AsyncRendering(USubstanceGraphInstance* GraphInstance)
{
	if (!GraphInstance || !GraphInstance->Instance || !GraphInstance->CanUpdate())
	{
		return;
	}

	Substance::Helpers::RenderAsync(GraphInstance->Instance);
}