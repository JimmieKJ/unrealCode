// Copyright 2016 Allegorithmic Inc. All rights reserved.
// File: SubstanceGraphInstance.cpp

#include "SubstanceCorePrivatePCH.h"
#include "SubstanceTexture2D.h"
#include "SubstanceInstanceFactory.h"
#include "SubstanceCoreHelpers.h"
#include "SubstanceImageInput.h"
#include "SubstanceImageInput.h"
#include "SubstanceStructuresSerialization.h"

#include <substance/framework/input.h>
#include <substance/framework/package.h>
#include <substance/framework/preset.h>

#if WITH_EDITOR
#include "ContentBrowserModule.h"
#endif

namespace sbsGraphInstance
{
	TArray<TWeakObjectPtr<USubstanceTexture2D>> SavedTextures;
}

USubstanceGraphInstance::USubstanceGraphInstance(class FObjectInitializer const & PCIP) : Super(PCIP)
{
	bDirtyCache = false;
	bCooked = false;
	bIsFrozen = false;
	Instance = NULL;
	CreatedMaterial = NULL;
}

void USubstanceGraphInstance::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	//If we aren't loading, always use most up to date serialization method
	if (!Ar.IsLoading())
	{
		Ar.UsingCustomVersion(FSubstanceCoreCustomVersion::GUID);
		SerializeCurrent(Ar);
		return;
	}

	//Check the version to see if we need to serialize legacy
	bool SerializeLegacy = false;
	if (Ar.CustomVer(FSubstanceCoreCustomVersion::GUID) < FSubstanceCoreCustomVersion::FrameworkRefactor)
	{
		//Handle Legacy Loading here
		SerializeLegacy = true;
	}

	//Register the most recent version with this AR
	Ar.UsingCustomVersion(FSubstanceCoreCustomVersion::GUID);

	//Call serialize based on version
	(SerializeLegacy == true) ? this->SerializeLegacy(Ar) : SerializeCurrent(Ar);
}

void USubstanceGraphInstance::SerializeCurrent(FArchive& Ar)
{
	//Save the instance index to link later
	Ar << PackageURL;

	//Does this load the object again or does this find the loaded object and assign the pointer?
	Ar << ParentFactory;

	if (Ar.IsSaving() && Instance /*&& !Ar.IsTransacting()*/)
	{
		SubstanceAir::Preset InstancePresets;
		InstancePresets.fill(*Instance);

		std::stringstream SS;
		SS << InstancePresets;

		uint32 presetsSize = SS.str().length();
		Ar << presetsSize;

		Ar.ByteOrderSerialize((void*)SS.str().c_str(), presetsSize);
	}
	else if (Ar.IsLoading())
	{
		uint32 presetsSize = 0;
		Ar << presetsSize;

		char* presetData = new char[presetsSize];

		Ar.ByteOrderSerialize(presetData, presetsSize);

		SubstanceAir::parsePreset(InstancePreset, presetData);
		delete[] presetData;
	}

	bCooked = Ar.IsCooking();
	Ar << bCooked;

	//Associate this asset with the current plugin version
	Ar.UsingCustomVersion(FSubstanceCoreCustomVersion::GUID);
	Ar.SetCustomVersion(FSubstanceCoreCustomVersion::GUID, FSubstanceCoreCustomVersion::LatestVersion, FName("LegacyUpdated"));
}

void USubstanceGraphInstance::SerializeLegacy(FArchive& Ar)
{
	//Serialize the old framework graph instance
	LegacySerailizeGraphInstance(Ar, this);

	//Serialize eventual fields from previous version
	if (Ar.IsLoading())
	{
		const int32 SbsCoreVer = GetLinkerCustomVersion(FSubstanceCoreCustomVersion::GUID);
		if (SbsCoreVer < FSubstanceCoreCustomVersion::FixedGraphFreeze)
		{
			int32 Padding;
			Ar << Padding;
			Ar << Padding;
		}
	}

	bCooked = Ar.IsCooking() && Ar.IsSaving();

	Ar << bCooked;
	Ar << ParentFactory;

	//Clear the archive to be updated with the new serialization on save.
	Ar.FlushCache();

	//Forcing package dirty
	UPackage* Package = GetOutermost();
	Package->SetDirtyFlag(true);
}

void USubstanceGraphInstance::CleanupGraphInstance()
{
	//Smoothly handle cleanup - clean up memory
	if (Instance)
	{
		//Reset Image Inputs
		Substance::Helpers::ResetGraphInstanceImageInputs(this);

		//Remove from queues and allows update to call delayed deletion.
		Substance::Helpers::ClearFromRender(this);

		//Disable all outputs
		auto ItOut = Instance->getOutputs().begin();
		for (; ItOut != Instance->getOutputs().end(); ++ItOut)
		{
			Substance::Helpers::Disable(*ItOut, false);
		}

		//Clean up the memory
		if (ParentFactory->IsValidLowLevel())
		{
			AIR_DELETE(Instance);
			Instance = NULL;
		}
	}
}

void USubstanceGraphInstance::BeginDestroy()
{
	//Route BeginDestroy
	Super::BeginDestroy();

	//Smoothly handle cleanup - clean up memory
	CleanupGraphInstance();

	//Unregister the graph
	if (ParentFactory)
		ParentFactory->UnregisterGraphInstance(this);
}

void USubstanceGraphInstance::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);

	if (!bDuplicateForPIE)
	{
		//After duplication, we need to recreate a parent instance and set it as outer
		//look for the original object, using the GUID
		USubstanceGraphInstance* RefInstance = NULL;

		for (TObjectIterator<USubstanceGraphInstance> It; It; ++It)
		{
			if (It->ParentFactory == ParentFactory && It->PackageURL == PackageURL)
			{
				RefInstance = (*It);
				break;
			}
		}

		check(RefInstance);

		Instance = AIR_NEW(SubstanceAir::GraphInstance)(RefInstance->Instance->mDesc);

		Substance::Helpers::CreateTextures(this);

		this->mUserData.ParentGraph = this;
		Instance->mUserData = (size_t)&mUserData;

		SubstanceAir::Preset CurrentRefGraphDeltas;
		CurrentRefGraphDeltas.fill(*RefInstance->Instance);
		CurrentRefGraphDeltas.apply(*Instance);

		Substance::Helpers::RenderAsync(Instance);

#if WITH_EDITOR
		if (GIsEditor)
		{
			UObject* MaterialParent = NULL;
			if (RefInstance->CreatedMaterial != NULL)
			{
				FName NewMaterialName = MakeUniqueObjectName(RefInstance->CreatedMaterial->GetOuter(),
					RefInstance->CreatedMaterial->GetClass(), FName(*RefInstance->CreatedMaterial->GetName()));

				UObject* MaterialBasePackage = CreatePackage(NULL, *RefInstance->CreatedMaterial->GetName());

				Substance::Helpers::CreateMaterial(this, NewMaterialName.ToString(), MaterialBasePackage);
			}

			TArray<UObject*> AssetList;
			for (auto itout = Instance->getOutputs().begin(); itout != Instance->getOutputs().end(); ++itout)
			{
				//Skip unsupported outputs
				if ((*itout)->mUserData == 0)
					continue;

				AssetList.AddUnique(reinterpret_cast<OutputInstanceData*>((*itout)->mUserData)->Texture.Get());
			}
			AssetList.AddUnique(this);
			if (CreatedMaterial.IsValid())
			{
				AssetList.AddUnique(CreatedMaterial.Get());
				UE_LOG(LogSubstanceCore, Warning, TEXT("Material Duplicated for - %s. Should now be added to asset list. Material Name - %s"),
					*GetName(), *CreatedMaterial->GetName());
			}

			FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
			ContentBrowserModule.Get().SyncBrowserToAssets(AssetList);
		}
#endif
	}
}

void USubstanceGraphInstance::PostLoad()
{
	bDirtyCache = false;

	//Make sure that the factory that created this is valid
	if (!ParentFactory)
	{
		UE_LOG(LogSubstanceCore, Error, TEXT("Impossible to find parent package for \"%s\". You should delete the object."),
			*GetName());
		Super::PostLoad();
		return;
	}

	//Make sure Substance package is valid for non-baked assets
	if (!ParentFactory->SubstancePackage && ParentFactory->GenerationMode != SGM_Baked)
	{
		UE_LOG(LogSubstanceCore, Error, TEXT("Parent factories substance package is invalid \"%s\"."),
			*GetName());
		Super::PostLoad();
		return;
	}

	//If this is baked, none of the post load setup needs to happen
	if (ParentFactory && ParentFactory->GenerationMode == SGM_Baked && bCooked)
	{
		Super::PostLoad();
		return;
	}

	ParentFactory->ConditionalPostLoad();
	ParentFactory->RegisterGraphInstance(this);

	if (Instance == NULL)
	{
		//Link the instance to one previously loaded
		if (this->LinkLoadedGraphInstance() == false)
		{
			UE_LOG(LogSubstanceCore, Error, TEXT("Unable to find a matching loaded graph for the Substance Graph Instance wrapper!!"));
			Super::PostLoad();
			return;
		}

		//Instantiate the user data
		mUserData.ParentGraph = this;
		mUserData.bHasPendingImageInputRendering = true;
		Instance->mUserData = (size_t)&mUserData;

		//Only finalize on legacy load
		if (!InstancePreset.mPackageUrl.size())
		{
			InstancePreset.mPackageUrl = Instance->mDesc.mPackageUrl;
			FinalizeLegacyPresets(this);
		}

		//Set instance presets
		if (InstancePreset.mPackageUrl == Instance->mDesc.mPackageUrl)
		{
			if (!InstancePreset.apply(*Instance) && InstancePreset.mInputValues.size() > 0)
				UE_LOG(LogSubstanceCore, Warning, TEXT("Failed to apply presets for instance - %s."), *GetName());
		}
	}

	//The input instances with the graph and their desc
	auto ItInInst(Instance->getInputs().begin());
	for (; ItInInst != Instance->getInputs().end(); ++ItInInst)
	{
		if ((*ItInInst)->mDesc.isImage())
		{
			SubstanceAir::InputInstanceImage* ImgInput = (SubstanceAir::InputInstanceImage*)(*ItInInst);
			if (!LinkImageInput(ImgInput))
			{
				FString InputLabel((*ItInInst)->mDesc.mLabel.c_str());
				UE_LOG(LogSubstanceCore, Warning, TEXT("Failed to find Image Input UAsset for - %s."), *InputLabel);
			}
		}
	}

	//Let the substance texture switch outputs back on
	auto ItOut(Instance->getOutputs().begin());
	for (; ItOut != Instance->getOutputs().end(); ++ItOut)
	{
		if ((*ItOut)->mUserData == 0 || reinterpret_cast<OutputInstanceData*>((*ItOut)->mUserData)->Texture.Get() == NULL)
		{
			(*ItOut)->mEnabled = false;
		}
	}

	if ((ParentFactory->GetGenerationMode() != ESubstanceGenerationMode::SGM_Baked && bCooked) || (bIsFrozen && bCooked))
	{
		Substance::Helpers::PushDelayedRender(this);
	}

	Super::PostLoad();
}

#if WITH_EDITOR
bool USubstanceGraphInstance::CanEditChange(const UProperty* InProperty) const
{
	return true;
}

void USubstanceGraphInstance::PreEditUndo()
{
	//Serialization of outputs does not include the shared ptr to textures,
	//save them before undo/redo

	auto itOut(Instance->getOutputs().begin());
	for (; itOut != Instance->getOutputs().end(); ++itOut)
	{
		sbsGraphInstance::SavedTextures.Add(reinterpret_cast<OutputInstanceData*>((*itOut)->mUserData)->Texture);
	}
}

void USubstanceGraphInstance::PostEditUndo()
{
	auto itOut(Instance->getOutputs().begin());
	TArray<TWeakObjectPtr<USubstanceTexture2D>>::TIterator SavedTexturesIt(sbsGraphInstance::SavedTextures);

	for (; itOut != Instance->getOutputs().end() && SavedTexturesIt; ++itOut, ++SavedTexturesIt)
	{
		reinterpret_cast<OutputInstanceData*>((*itOut)->mUserData)->Texture = (*SavedTexturesIt).Get();
	}

	sbsGraphInstance::SavedTextures.Empty();

	Substance::Helpers::RenderAsync(Instance);
}
#endif //WITH_EDITOR

bool USubstanceGraphInstance::CanUpdate()
{
	if (ParentFactory->GetGenerationMode() == ESubstanceGenerationMode::SGM_Baked)
	{
		UE_LOG(LogSubstanceCore, Warning, TEXT("Cannot modify Graph Instance \"%s\", parent instance factory is baked."),
			*ParentFactory->GetName());
		return false;
	}

	if (bIsFrozen)
	{
		UE_LOG(LogSubstanceCore, Warning, TEXT("Cannot modify Graph Instance \"%s\", instance is frozen."),
			*GetName());
		return false;
	}

	return true;
}

bool USubstanceGraphInstance::SetInputImg(const FString& Name, class UObject* Value)
{
	if (Instance)
	{
		if (Substance::Helpers::IsSupportedImageInput(Value))
		{
			UObject* PrevSource = GetInputImg(Name);

			Substance::Helpers::UpdateInput(this->Instance, Instance->mInstanceUid, Value);

			bool bUseOtherInput = false;
			if (PrevSource)
			{
				for (auto InputName : GetInputNames())
				{
					if (PrevSource == GetInputImg(InputName))
					{
						bUseOtherInput = true;
					}
				}
			}

			USubstanceImageInput* PrevImageInput = Cast<USubstanceImageInput>(PrevSource);
			USubstanceImageInput* NewImageInput = Cast<USubstanceImageInput>(Value);
			if (!bUseOtherInput)
			{
				const uint32* Key = ImageSources.FindKey(PrevImageInput);
				if (Key)
				{
					ImageSources[*Key] = NewImageInput;
				}
			}

			Substance::Helpers::RenderAsync(Instance);
			return true;
		}
	}

	return false;
}

class UObject* USubstanceGraphInstance::GetInputImg(const FString& Name)
{
	auto ItInInst(Instance->getInputs().begin());
	for (; ItInInst != Instance->getInputs().end(); ++ItInInst)
	{
		if ((*ItInInst)->mDesc.mIdentifier.c_str() == Name && (*ItInInst)->mDesc.mType == Substance_IType_Image)
		{
			SubstanceAir::InputInstanceImage* TypedInst = (SubstanceAir::InputInstanceImage*)(*ItInInst);
			UObject* CurrentSource = reinterpret_cast<InputImageData*>(TypedInst->getImage()->mUserData)->ImageUObjectSource;
			return CurrentSource;
		}
	}

	return NULL;
}

TArray<FString> USubstanceGraphInstance::GetInputNames()
{
	TArray<FString> Names;

	auto ItInInst(Instance->getInputs().begin());
	for (; ItInInst != Instance->getInputs().end(); ++ItInInst)
	{
		Names.Add((*ItInInst)->mDesc.mIdentifier.c_str());
	}

	return Names;
}

ESubstanceInputType USubstanceGraphInstance::GetInputType(FString InputName)
{
	auto ItInInst(Instance->getInputs().begin());
	for (; ItInInst != Instance->getInputs().end(); ++ItInInst)
	{
		if ((*ItInInst)->mDesc.mIdentifier.c_str() == InputName)
		{
			return (ESubstanceInputType)((*ItInInst)->mDesc.mType);
		}
	}

	return ESubstanceInputType::SIT_MAX;
}

void USubstanceGraphInstance::SetInputInt(FString IntputName, const TArray<int32>& InputValues)
{
	if (Instance)
	{
		//Loop through our inputs and try and find a match
		for (std::size_t i = 0; i < Instance->getInputs().size(); ++i)
		{
			SubstanceAir::InputInstanceBase* CurrentInput = Instance->getInputs()[i];
			if (CurrentInput->mDesc.mIdentifier.c_str() == IntputName && CurrentInput->mDesc.isNumerical())
			{
				Substance::Helpers::SetNumericalInputValue((SubstanceAir::InputInstanceNumericalBase*)CurrentInput, InputValues);
				Substance::Helpers::UpdateInput(this->Instance, CurrentInput->mDesc.mUid, InputValues);
				break;
			}
		}
	}
}

void USubstanceGraphInstance::SetInputFloat(FString IntputName, const TArray<float>& InputValues)
{
	if (Instance)
	{
		//Loop through our inputs and try and find a match
		for (std::size_t i = 0; i < Instance->getInputs().size(); ++i)
		{
			SubstanceAir::InputInstanceBase* CurrentInput = Instance->getInputs()[i];
			if (CurrentInput->mDesc.mIdentifier.c_str() == IntputName && CurrentInput->mDesc.isNumerical())
			{
				Substance::Helpers::SetNumericalInputValue((SubstanceAir::InputInstanceNumericalBase*)CurrentInput, InputValues);
				Substance::Helpers::UpdateInput(this->Instance, CurrentInput->mDesc.mUid, InputValues);
				break;
			}
		}
	}
}

TArray<int32> USubstanceGraphInstance::GetInputInt(FString IntputName)
{
	TArray< int32 > DummyValue;

	auto ItInInst(Instance->getInputs().begin());
	for (; ItInInst != Instance->getInputs().end(); ++ItInInst)
	{
		if ((*ItInInst)->mDesc.mIdentifier.c_str() == IntputName &&
			((*ItInInst)->mDesc.mType == Substance_IType_Integer ||
			(*ItInInst)->mDesc.mType == Substance_IType_Integer2 ||
				(*ItInInst)->mDesc.mType == Substance_IType_Integer3 ||
				(*ItInInst)->mDesc.mType == Substance_IType_Integer4))
		{
			return Substance::Helpers::GetValueInt(*(*ItInInst));
		}
	}

	return DummyValue;
}

TArray<float> USubstanceGraphInstance::GetInputFloat(FString IntputName)
{
	TArray<float> DummyValue;

	auto ItInInst = Instance->getInputs().begin();
	for (; ItInInst != Instance->getInputs().end(); ++ItInInst)
	{
		if ((*ItInInst)->mDesc.mIdentifier.c_str() == IntputName &&
			((*ItInInst)->mDesc.mType == Substance_IType_Float ||
			(*ItInInst)->mDesc.mType == Substance_IType_Float2 ||
				(*ItInInst)->mDesc.mType == Substance_IType_Float3 ||
				(*ItInInst)->mDesc.mType == Substance_IType_Float4))
		{
			return Substance::Helpers::GetValueFloat(*(*ItInInst));
		}
	}

	return DummyValue;
}

using namespace Substance;

FSubstanceFloatInputDesc USubstanceGraphInstance::GetFloatInputDesc(FString IntputName)
{
	using namespace SubstanceAir;
	FSubstanceFloatInputDesc K2_InputDesc;

	auto ItInInst(Instance->getInputs().begin());
	for (; ItInInst != Instance->getInputs().end(); ++ItInInst)
	{
		if ((*ItInInst)->mDesc.mIdentifier.c_str() == IntputName &&
			((*ItInInst)->mDesc.mType == Substance_IType_Float ||
			(*ItInInst)->mDesc.mType == Substance_IType_Float2 ||
				(*ItInInst)->mDesc.mType == Substance_IType_Float3 ||
				(*ItInInst)->mDesc.mType == Substance_IType_Float4))
		{
			const SubstanceAir::InputDescBase * InputDesc = &(*ItInInst)->mDesc;

			K2_InputDesc.Name = IntputName;

			switch ((*ItInInst)->mDesc.mType)
			{
			case Substance_IType_Float:
				K2_InputDesc.Min.Add(((InputDescFloat*)InputDesc)->mMaxValue);
				K2_InputDesc.Max.Add(((InputDescFloat*)InputDesc)->mMaxValue);
				K2_InputDesc.Default.Add(((InputDescFloat*)InputDesc)->mDefaultValue);
				break;
			case Substance_IType_Float2:
				K2_InputDesc.Min.Add(((InputDescFloat2*)InputDesc)->mMaxValue.x);
				K2_InputDesc.Min.Add(((InputDescFloat2*)InputDesc)->mMaxValue.y);

				K2_InputDesc.Max.Add(((InputDescFloat2*)InputDesc)->mMaxValue.x);
				K2_InputDesc.Max.Add(((InputDescFloat2*)InputDesc)->mMaxValue.y);

				K2_InputDesc.Default.Add(((InputDescFloat2*)InputDesc)->mDefaultValue.x);
				K2_InputDesc.Default.Add(((InputDescFloat2*)InputDesc)->mDefaultValue.y);
				break;
			case Substance_IType_Float3:
				K2_InputDesc.Min.Add(((InputDescFloat3*)InputDesc)->mMaxValue.x);
				K2_InputDesc.Min.Add(((InputDescFloat3*)InputDesc)->mMaxValue.y);
				K2_InputDesc.Min.Add(((InputDescFloat3*)InputDesc)->mMaxValue.z);

				K2_InputDesc.Max.Add(((InputDescFloat3*)InputDesc)->mMaxValue.x);
				K2_InputDesc.Max.Add(((InputDescFloat3*)InputDesc)->mMaxValue.y);
				K2_InputDesc.Max.Add(((InputDescFloat3*)InputDesc)->mMaxValue.z);

				K2_InputDesc.Default.Add(((InputDescFloat3*)InputDesc)->mDefaultValue.x);
				K2_InputDesc.Default.Add(((InputDescFloat3*)InputDesc)->mDefaultValue.y);
				K2_InputDesc.Default.Add(((InputDescFloat3*)InputDesc)->mDefaultValue.z);
				break;
			case Substance_IType_Float4:
				K2_InputDesc.Min.Add(((InputDescFloat4*)InputDesc)->mMaxValue.x);
				K2_InputDesc.Min.Add(((InputDescFloat4*)InputDesc)->mMaxValue.y);
				K2_InputDesc.Min.Add(((InputDescFloat4*)InputDesc)->mMaxValue.z);
				K2_InputDesc.Min.Add(((InputDescFloat4*)InputDesc)->mMaxValue.w);

				K2_InputDesc.Max.Add(((InputDescFloat4*)InputDesc)->mMaxValue.x);
				K2_InputDesc.Max.Add(((InputDescFloat4*)InputDesc)->mMaxValue.y);
				K2_InputDesc.Max.Add(((InputDescFloat4*)InputDesc)->mMaxValue.z);
				K2_InputDesc.Min.Add(((InputDescFloat4*)InputDesc)->mMaxValue.w);

				K2_InputDesc.Default.Add(((InputDescFloat4*)InputDesc)->mDefaultValue.x);
				K2_InputDesc.Default.Add(((InputDescFloat4*)InputDesc)->mDefaultValue.y);
				K2_InputDesc.Default.Add(((InputDescFloat4*)InputDesc)->mDefaultValue.z);
				K2_InputDesc.Default.Add(((InputDescFloat4*)InputDesc)->mDefaultValue.w);
				break;
			}
		}
	}

	return K2_InputDesc;
}

FSubstanceIntInputDesc USubstanceGraphInstance::GetIntInputDesc(FString IntputName)
{
	using namespace SubstanceAir;
	FSubstanceIntInputDesc K2_InputDesc;

	auto ItInInst(Instance->getInputs().begin());
	for (; ItInInst != Instance->getInputs().end(); ++ItInInst)
	{
		if ((*ItInInst)->mDesc.mIdentifier.c_str() == IntputName &&
			((*ItInInst)->mDesc.mType == Substance_IType_Integer ||
			(*ItInInst)->mDesc.mType == Substance_IType_Integer2 ||
				(*ItInInst)->mDesc.mType == Substance_IType_Integer3 ||
				(*ItInInst)->mDesc.mType == Substance_IType_Integer4))
		{
			const SubstanceAir::InputDescBase* InputDesc = &(*ItInInst)->mDesc;

			K2_InputDesc.Name = IntputName;

			switch ((*ItInInst)->mDesc.mType)
			{
			case Substance_IType_Integer:
				K2_InputDesc.Min.Add(((SubstanceAir::InputDescInt*)InputDesc)->mMinValue);
				K2_InputDesc.Max.Add(((SubstanceAir::InputDescInt*)InputDesc)->mMaxValue);
				K2_InputDesc.Default.Add(((SubstanceAir::InputDescInt*)InputDesc)->mDefaultValue);
				break;
			case Substance_IType_Integer2:
				K2_InputDesc.Min.Add(((SubstanceAir::InputDescInt2*)InputDesc)->mMinValue.x);
				K2_InputDesc.Min.Add(((SubstanceAir::InputDescInt2*)InputDesc)->mMinValue.y);

				K2_InputDesc.Max.Add(((SubstanceAir::InputDescInt2*)InputDesc)->mMaxValue.x);
				K2_InputDesc.Max.Add(((SubstanceAir::InputDescInt2*)InputDesc)->mMaxValue.y);

				K2_InputDesc.Default.Add(((SubstanceAir::InputDescInt2*)InputDesc)->mDefaultValue.x);
				K2_InputDesc.Default.Add(((SubstanceAir::InputDescInt2*)InputDesc)->mDefaultValue.y);
				break;
			case Substance_IType_Integer3:
				K2_InputDesc.Min.Add(((SubstanceAir::InputDescInt3*)InputDesc)->mMinValue.x);
				K2_InputDesc.Min.Add(((SubstanceAir::InputDescInt3*)InputDesc)->mMinValue.y);
				K2_InputDesc.Min.Add(((SubstanceAir::InputDescInt3*)InputDesc)->mMinValue.z);

				K2_InputDesc.Max.Add(((SubstanceAir::InputDescInt3*)InputDesc)->mMaxValue.x);
				K2_InputDesc.Max.Add(((SubstanceAir::InputDescInt3*)InputDesc)->mMaxValue.y);
				K2_InputDesc.Max.Add(((SubstanceAir::InputDescInt3*)InputDesc)->mMaxValue.z);

				K2_InputDesc.Default.Add(((SubstanceAir::InputDescInt3*)InputDesc)->mDefaultValue.x);
				K2_InputDesc.Default.Add(((SubstanceAir::InputDescInt3*)InputDesc)->mDefaultValue.y);
				K2_InputDesc.Default.Add(((SubstanceAir::InputDescInt3*)InputDesc)->mDefaultValue.z);
				break;
			case Substance_IType_Integer4:
				K2_InputDesc.Min.Add(((SubstanceAir::InputDescInt4*)InputDesc)->mMinValue.x);
				K2_InputDesc.Min.Add(((SubstanceAir::InputDescInt4*)InputDesc)->mMinValue.y);
				K2_InputDesc.Min.Add(((SubstanceAir::InputDescInt4*)InputDesc)->mMinValue.z);
				K2_InputDesc.Min.Add(((SubstanceAir::InputDescInt4*)InputDesc)->mMinValue.w);

				K2_InputDesc.Max.Add(((SubstanceAir::InputDescInt4*)InputDesc)->mMaxValue.x);
				K2_InputDesc.Max.Add(((SubstanceAir::InputDescInt4*)InputDesc)->mMaxValue.y);
				K2_InputDesc.Max.Add(((SubstanceAir::InputDescInt4*)InputDesc)->mMaxValue.z);
				K2_InputDesc.Max.Add(((SubstanceAir::InputDescInt4*)InputDesc)->mMaxValue.w);

				K2_InputDesc.Default.Add(((SubstanceAir::InputDescInt4*)InputDesc)->mDefaultValue.x);
				K2_InputDesc.Default.Add(((SubstanceAir::InputDescInt4*)InputDesc)->mDefaultValue.y);
				K2_InputDesc.Default.Add(((SubstanceAir::InputDescInt4*)InputDesc)->mDefaultValue.z);
				K2_InputDesc.Default.Add(((SubstanceAir::InputDescInt4*)InputDesc)->mDefaultValue.w);
				break;
			}
		}
	}

	return K2_InputDesc;
}

FSubstanceInstanceDesc USubstanceGraphInstance::GetInstanceDesc()
{
	FSubstanceInstanceDesc K2_InstanceDesc;
	K2_InstanceDesc.Name = Instance->mDesc.mLabel.c_str();
	return K2_InstanceDesc;
}

bool USubstanceGraphInstance::LinkLoadedGraphInstance()
{
	auto Itr = ParentFactory->SubstancePackage->getGraphs().begin();
	for (; Itr != ParentFactory->SubstancePackage->getGraphs().end(); ++Itr)
	{
		if (Itr->mPackageUrl.c_str() == PackageURL)
		{
			Instance = AIR_NEW(SubstanceAir::GraphInstance)(*Itr);
			break;
		}
	}

	if (Instance == NULL)
	{
		UE_LOG(LogSubstanceCore, Error, TEXT("Could not find a matching desc to for graph instance %s"), *GetName());
		return false;
	}

	return true;
}

bool USubstanceGraphInstance::LinkImageInput(SubstanceAir::InputInstanceImage* ImageInput)
{
	//Check the map to see if we have the input
	USubstanceImageInput* SubstanceImage = NULL;
	if (ImageSources.Find(ImageInput->mDesc.mUid))
		SubstanceImage = *ImageSources.Find(ImageInput->mDesc.mUid);

	if (SubstanceImage == NULL)
		return false;

	//Sets the image input
	Substance::Helpers::SetImageInput(ImageInput, SubstanceImage, Instance);

	//Make sure the image is loaded
	SubstanceImage->ConditionalPostLoad();

	//Delay the set image input, the source is not necessarily ready
	Substance::Helpers::PushDelayedImageInput(ImageInput, Instance);
	mUserData.bHasPendingImageInputRendering = true;

	return true;
}