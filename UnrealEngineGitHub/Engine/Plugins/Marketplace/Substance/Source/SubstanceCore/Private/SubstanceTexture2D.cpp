// Copyright 2016 Allegorithmic Inc. All rights reserved.
// File: SubstanceTexture2D.cpp

#include "SubstanceCorePrivatePCH.h"
#include "SubstanceInstanceFactory.h"
#include "SubstanceCoreHelpers.h"
#include "SubstanceTexture2D.h"
#include "SubstanceSettings.h"
#include "SubstanceTexture2DDynamicResource.h"
#include "substance/framework/output.h"

#if WITH_EDITOR
#include "ObjectTools.h"
#include "ContentBrowserModule.h"
#include "TargetPlatform.h"
#include "ModuleManager.h"
#endif //WITH_EDITOR

#if PLATFORM_PS4 || (WITH_EDITOR && SUBSTANCE_FOR_PS4)
#include "SubstanceCoreConsoleSpecific.h"
#endif

USubstanceTexture2D::USubstanceTexture2D(class FObjectInitializer const & PCIP) : Super(PCIP)
{
	bLinkLegacy = false;
}

FString USubstanceTexture2D::GetDesc()
{
	return FString::Printf(TEXT("%dx%d[%s]"), SizeX, SizeY, GPixelFormats[Format].Name);
}

void USubstanceTexture2D::Serialize(FArchive& Ar)
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
	bool ShouldSerializeLegacy = false;
	if (Ar.CustomVer(FSubstanceCoreCustomVersion::GUID) < FSubstanceCoreCustomVersion::FrameworkRefactor)
	{
		//Handle Legacy Loading here
		bLinkLegacy = true;
		ShouldSerializeLegacy = true;
	}

	//Register the new version of the engine.
	Ar.UsingCustomVersion(FSubstanceCoreCustomVersion::GUID);

	//Call serialize based on version
	(ShouldSerializeLegacy == true) ? SerializeLegacy(Ar) : SerializeCurrent(Ar);
}

void USubstanceTexture2D::SerializeCurrent(FArchive& Ar)
{
	//TODO:: Remove filler before release
	int32 Filler;

	Ar << Format;
	Ar << SizeX;
	Ar << SizeY;
	Ar << NumMips;
	Ar << mUid;
	Ar << ParentInstance;
	Ar << Filler;
	Ar << mUserData.CacheGuid;

	int32 FirstMipToSerialize = 0;

	bCooked = Ar.IsCooking();
	Ar << bCooked;

	if (bCooked && !this->IsDefaultSubobject())
	{
		ESubstanceGenerationMode ParentPackageGenerationMode = ESubstanceGenerationMode::SGM_PlatformDefault;

		if (ParentInstance && ParentInstance->ParentFactory)
		{
			ParentPackageGenerationMode = ParentInstance->ParentFactory->GetGenerationMode();
			if (ParentPackageGenerationMode == ESubstanceGenerationMode::SGM_PlatformDefault)
			{
				ParentPackageGenerationMode = GetDefault<USubstanceSettings>()->DefaultGenerationMode;
			}
		}
		else
		{
			ParentPackageGenerationMode = ESubstanceGenerationMode::SGM_Baked;
		}

		if (Ar.IsSaving() || Ar.IsCooking())
		{
			//The number of mipmap levels which will not be saved / regenerated during loading
			int32 MipsToRegen = 0;

			switch (ParentPackageGenerationMode)
			{
			//Baked will not regenerate any of the mips.
			case ESubstanceGenerationMode::SGM_Baked:
			{
				MipsToRegen = 0;
				break;
			}

			//Load Sync will regenerate all of the maps at load time. This is a blocking call and to keep load times down, recommended that
			//this is used sparsely and elements that will have run time input changes or are core game meshes (characters)
			case ESubstanceGenerationMode::SGM_OnLoadSync:
			case ESubstanceGenerationMode::SGM_OnLoadSyncAndCache:
			{
				MipsToRegen = NumMips - 1;
				break;
			}

			//Load Async will generate all mip levels based on project settings. This allows for substances to load small data amounts
			//when many load operations are happening and after a few frames, regenerate so all mips. Used to optimize load times.
			case ESubstanceGenerationMode::SGM_OnLoadAsync:
			case ESubstanceGenerationMode::SGM_OnLoadAsyncAndCache:
			{
				MipsToRegen = FMath::Clamp(GetDefault<USubstanceSettings>()->AsyncLoadMipClip, 0, NumMips - 1);
				break;
			}

			//Default to regenerating all maps.
			default:
			{
				MipsToRegen = NumMips - 1;
				break;
			}
			}

			FirstMipToSerialize += FMath::Max(0, MipsToRegen);
		}

		Ar << FirstMipToSerialize;
	}

	if (Ar.IsSaving() && !this->IsDefaultSubobject())
	{
#if WITH_EDITOR && SUBSTANCE_FOR_PS4
		//Special case for the ps4
		if (Ar.IsCooking() && Ar.CookingTarget()->PlatformName() == FString(TEXT("PS4")) && !this->IsDefaultSubobject())
		{
			if (!PlatformMips.Num())
			{
				int32 LargestMipSize = Mips[FirstMipToSerialize].BulkData.GetBulkDataSize();
				void* TempPtr = FMemory::Malloc(LargestMipSize);

				FTexture2DMipMap* MipMap;

				for (int32 MipIndex = 0; MipIndex < NumMips - FirstMipToSerialize; ++MipIndex)
				{
					MipMap = new(PlatformMips) FTexture2DMipMap;
					FTexture2DMipMap& MipReference = Mips[MipIndex + FirstMipToSerialize];

					MipReference.BulkData.GetCopy(&TempPtr, true);
					MipMap->SizeX = MipReference.SizeX;
					MipMap->SizeY = MipReference.SizeY;

					Substance::Helpers::TileMipForPS4(TempPtr, MipMap, Format);
				}
				FMemory::Free(TempPtr);
			}

			for (int32 MipIndex = 0; MipIndex < PlatformMips.Num(); ++MipIndex)
			{
				PlatformMips[MipIndex].Serialize(Ar, this, MipIndex);
			}
		}
		else
		{
			for (int32 MipIndex = 0; MipIndex < NumMips - FirstMipToSerialize; ++MipIndex)
			{
				Mips[MipIndex + FirstMipToSerialize].Serialize(Ar, this, MipIndex);
			}
		}
#else

		for (int32 MipIndex = 0; MipIndex < NumMips - FirstMipToSerialize; ++MipIndex)
		{
			Mips[MipIndex + FirstMipToSerialize].Serialize(Ar, this, MipIndex);
		}
#endif

	}
	else if (Ar.IsLoading())
	{
		NumMips -= FirstMipToSerialize;
		Mips.Empty(NumMips);

		for (int32 MipIndex = 0; MipIndex < NumMips; ++MipIndex)
		{
			new(Mips) FTexture2DMipMap();
			Mips[MipIndex].Serialize(Ar, this, MipIndex);
		}
	}

	//Associate this asset with the current plugin version
	Ar.UsingCustomVersion(FSubstanceCoreCustomVersion::GUID);
	Ar.SetCustomVersion(FSubstanceCoreCustomVersion::GUID, FSubstanceCoreCustomVersion::LatestVersion, FName("LegacyUpdated"));
}

void USubstanceTexture2D::SerializeLegacy(FArchive& Ar)
{
	Ar << Format;
	Ar << SizeX;
	Ar << SizeY;
	Ar << NumMips;
	Ar << mUserData.CacheGuid;
	Ar << ParentInstance;

	int32 FirstMipToSerialize = 0;

	bCooked = Ar.IsCooking();
	Ar << bCooked;

	if (bCooked && !this->IsDefaultSubobject())
	{
		ESubstanceGenerationMode ParentPackageGenerationMode = ESubstanceGenerationMode::SGM_PlatformDefault;

		if (ParentInstance && ParentInstance->ParentFactory)
		{
			ParentPackageGenerationMode = ParentInstance->ParentFactory->GetGenerationMode();
			if (ParentPackageGenerationMode == ESubstanceGenerationMode::SGM_PlatformDefault)
			{
				ParentPackageGenerationMode = GetDefault<USubstanceSettings>()->DefaultGenerationMode;
			}
		}

		Ar << FirstMipToSerialize;
	}

	NumMips -= FirstMipToSerialize;
	Mips.Empty(NumMips);

	for (int32 MipIndex = 0; MipIndex < NumMips; ++MipIndex)
	{
		new(Mips) FTexture2DMipMap();
		Mips[MipIndex].Serialize(Ar, this, MipIndex);
	}

	//Clear the archive to be updated with the new serialization on save.
	Ar.FlushCache();

	//Forcing package dirty
	UPackage* Package = GetOutermost();
	Package->SetDirtyFlag(true);
}

void USubstanceTexture2D::BeginDestroy()
{
	//Route BeginDestroy.
	Super::BeginDestroy();

	if (OutputCopy)
	{
		//Reset output user data
		OutputCopy->mUserData = 0;

		//Nullify the pointer to this texture's address so that others see it has been destroyed
		mUserData.Texture.Reset();

		//Disable the output in the parent instance
		if (ParentInstance && ParentInstance->Instance)
		{
			Substance::Helpers::ClearFromRender(ParentInstance);

			auto ItOut = ParentInstance->Instance->getOutputs().begin();
			for (; ItOut != ParentInstance->Instance->getOutputs().end(); ++ItOut)
			{
				if ((*ItOut)->mDesc.mUid == mUid)
				{
					(*ItOut)->mEnabled = false;
					break;
				}
			}
		}
	}
}

ESubChannelType USubstanceTexture2D::GetChannel()
{
	switch (OutputCopy->mDesc.mChannel) {
	case SubstanceAir::Channel_BaseColor:		 return ESubChannelType::BaseColor;
	case SubstanceAir::Channel_Metallic:		 return ESubChannelType::Metallic;
	case SubstanceAir::Channel_Roughness:		 return ESubChannelType::Roughness;
	case SubstanceAir::Channel_Emissive:		 return ESubChannelType::Emissive;
	case SubstanceAir::Channel_Normal:			 return ESubChannelType::Normal;
	case SubstanceAir::Channel_Mask:			 return ESubChannelType::Mask;
	case SubstanceAir::Channel_Opacity:			 return ESubChannelType::Opacity;
	case SubstanceAir::Channel_Refraction:		 return ESubChannelType::Refraction;
	case SubstanceAir::Channel_AmbientOcclusion: return ESubChannelType::AmbientOcclusion;
	default:									 return ESubChannelType::Invalid;
	};
}

#if WITH_EDITOR
bool USubstanceTexture2D::CanEditChange(const UProperty* InProperty) const
{
	bool bIsEditable = Super::CanEditChange(InProperty);

	if (bIsEditable && InProperty != NULL)
	{
		bIsEditable = false;

		if (InProperty->GetFName() == TEXT("AddressX") ||
			InProperty->GetFName() == TEXT("AddressY") ||
			InProperty->GetFName() == TEXT("UnpackMin") ||
			InProperty->GetFName() == TEXT("UnpackMax") ||
			InProperty->GetFName() == TEXT("Filter") ||
			InProperty->GetFName() == TEXT("LODBias") ||
			InProperty->GetFName() == TEXT("sRGB") ||
			InProperty->GetFName() == TEXT("LODGroup"))
		{
			bIsEditable = true;
		}
	}

	return bIsEditable;
}
#endif

void USubstanceTexture2D::LinkOutputInstance()
{
	//Find the output using the UID serialized
	OutputCopy = NULL;
	auto OutIt = ParentInstance->Instance->getOutputs().begin();
	for (; OutIt != ParentInstance->Instance->getOutputs().end(); ++OutIt)
	{
		//Check to verify the substance output instance has a valid ID.
		if ((*OutIt)->mDesc.mUid == 0)
		{
			UE_LOG(LogSubstanceCore, Error, TEXT("(%s) Output instance has an invalid UID"), (*OutIt)->mDesc.mLabel.c_str());
		}

		if ((*OutIt)->mDesc.mUid == mUid)
		{
			OutputCopy = const_cast<SubstanceAir::OutputInstance*>(*OutIt);
			break;
		}
	}
}

void USubstanceTexture2D::LinkLegacyOutputInstance()
{
	//Find the output using the UID serialized
	OutputCopy = NULL;
	auto OutIt = ParentInstance->Instance->getOutputs().begin();
	for (; OutIt != ParentInstance->Instance->getOutputs().end(); ++OutIt)
	{
		// Search the map for the GUID using the mUid as the key
		if (ParentInstance->OutputTextureLinkData[(*OutIt)->mDesc.mUid] == mUserData.CacheGuid)
		{
			OutputCopy = const_cast<SubstanceAir::OutputInstance*>(*OutIt);
			mUid = OutputCopy->mDesc.mUid;
			break;
		}
	}
}

void USubstanceTexture2D::PostLoad()
{
	//Make sure our parent is valid
	if (NULL == ParentInstance)
	{
		UE_LOG(LogSubstanceCore, Log, TEXT("No parent instance found for this SubstanceTexture2D (%s). Defaulting to baked texture."), *GetFullName());
		Super::PostLoad();
		return;
	}

	//Make sure the parent instance is loaded
	ParentInstance->ConditionalPostLoad();

	//Make sure the graph instance is valid for non baked assets
	if (NULL == ParentInstance->Instance && ParentInstance->ParentFactory->GenerationMode != SGM_Baked)
	{
		UE_LOG(LogSubstanceCore, Error, TEXT("Framework graph instance of (%s) is null - Texture 2D Not loaded correctly "), *GetFullName());
		Super::PostLoad();
		return;
	}

	//Don't run the setup for baked assets
	if (ParentInstance->ParentFactory->GenerationMode == SGM_Baked && bCooked)
	{
		Super::PostLoad();
		return;
	}

	//Finds our output instance and stores a reference to class member
	(bLinkLegacy == true) ? LinkLegacyOutputInstance() : LinkOutputInstance();

	if (!OutputCopy)
	{
		//The opposite situation is possible, no texture but an OutputInstance,
		//in this case the OutputInstance is disabled, but when the texture is
		//alive the Output must exist.
		UE_LOG(LogSubstanceCore, Error, TEXT("No matching output found for this SubstanceTexture2D (%s). You need to delete the texture and its parent instance."), *GetFullName());
		Super::PostLoad();
		return;
	}
	else
	{
		mUserData.Texture = this;
		mUserData.ParentInstance = ParentInstance;
		OutputCopy->mUserData = (size_t)&mUserData;
		OutputCopy->mEnabled = true;

		if (ParentInstance->ParentFactory->GetGenerationMode() != ESubstanceGenerationMode::SGM_Baked)
		{
			OutputCopy->invalidate();
		}

		//Revalidate format in the case things have changes since this asset was created
		if (ParentInstance && ParentInstance->ParentFactory)
			Substance::Helpers::ValidateFormat(this, OutputCopy);
	}

	//Set PS4 tiling 
#if PLATFORM_PS4
	bNoTiling = false;
#endif

	Super::PostLoad();
}

void USubstanceTexture2D::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);

	if (!bDuplicateForPIE)
	{
		USubstanceTexture2D* RefTexture = NULL;

		for (TObjectIterator<USubstanceTexture2D> It; It; ++It)
		{
			if ((*It)->mUid == mUid && *It != this)
			{
				RefTexture = *It;
				break;
			}
		}
		check(RefTexture);

		//After duplication, we need to recreate a parent instance
		//look for the original object, using the GUID
		TWeakObjectPtr<USubstanceGraphInstance> RefInstance = RefTexture->ParentInstance;

		if (!RefInstance.IsValid() || !RefInstance->Instance)
		{
			return;
		}

		USubstanceGraphInstance* NewGraphInstance = Substance::Helpers::DuplicateGraphInstance(RefInstance.Get());

		//Now we need to bind this to the new instance and bind its output to this
		ParentInstance = NewGraphInstance;

		auto ItOut = NewGraphInstance->Instance->getOutputs().begin();
		for (; ItOut != NewGraphInstance->Instance->getOutputs().end(); ++ItOut)
		{
			if ((*ItOut)->mDesc.mUid == RefTexture->OutputCopy->mDesc.mUid)
			{
				mUserData.ParentInstance = ParentInstance;
				mUserData.Texture = this;
				mUserData.CacheGuid = FGuid::NewGuid();

				(*ItOut)->mEnabled = true;
				(*ItOut)->mUserData = (size_t)&mUserData;

				this->mUid = (*ItOut)->mDesc.mUid;

				reinterpret_cast<OutputInstanceData*>((*ItOut)->mUserData)->Texture = this;

				OutputCopy = (*ItOut);
				break;
			}
		}

		Substance::Helpers::RenderAsync(NewGraphInstance->Instance);

#if WITH_EDITOR
		if (GIsEditor)
		{
			TArray<UObject*> AssetList;
			auto itout = NewGraphInstance->Instance->getOutputs().begin();
			for (; itout != NewGraphInstance->Instance->getOutputs().end(); ++itout)
			{
				AssetList.AddUnique(reinterpret_cast<OutputInstanceData*>((*itout)->mUserData)->Texture.Get());
			}
			AssetList.AddUnique(NewGraphInstance);

			FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
			ContentBrowserModule.Get().SyncBrowserToAssets(AssetList);
		}
#endif
	}
}

void USubstanceTexture2D::GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize)
{
	for (auto it = Mips.CreateConstIterator(); it; ++it)
	{
		CumulativeResourceSize.AddDedicatedSystemMemoryBytes(it->BulkData.GetBulkDataSize());
	}
}

FTextureResource* USubstanceTexture2D::CreateResource()
{
	if (Mips.Num())
	{
		return new FSubstanceTexture2DDynamicResource(this);
	}

	return NULL;
}

void USubstanceTexture2D::UpdateResource()
{
	Super::UpdateResource();

	if (Resource)
	{
		struct FUpdateSubstanceTexture
		{
			FTexture2DDynamicResource* Resource;
			USubstanceTexture2D* Owner;
			TArray<const void*>	MipData;
		};

		FUpdateSubstanceTexture* SubstanceData = new FUpdateSubstanceTexture;

		SubstanceData->Resource = (FTexture2DDynamicResource*)Resource;
		SubstanceData->Owner = this;

		for (int32 MipIndex = 0; MipIndex < SubstanceData->Owner->Mips.Num(); MipIndex++)
		{
			FTexture2DMipMap& MipMap = SubstanceData->Owner->Mips[MipIndex];

			//This gets unlocked in render command
			const void* MipData = MipMap.BulkData.LockReadOnly();
			SubstanceData->MipData.Add(MipData);
		}

		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			UpdateSubstanceTexture,
			FUpdateSubstanceTexture*, SubstanceData, SubstanceData,
			{
				//Read the resident mip-levels into the RHI texture.
				for (int32 MipIndex = 0; MipIndex < SubstanceData->Owner->Mips.Num(); MipIndex++)
				{
					uint32 DestPitch;
					void* TheMipData = RHILockTexture2D(SubstanceData->Resource->GetTexture2DRHI(), MipIndex, RLM_WriteOnly, DestPitch, false);

					FTexture2DMipMap& MipMap = SubstanceData->Owner->Mips[MipIndex];
					const void* MipData = SubstanceData->MipData[MipIndex];

					//For platforms that returned 0 pitch from Lock, we need to just use the bulk data directly, never do
					//runtime block size checking, conversion, or the like
					if (DestPitch == 0)
					{
						FMemory::Memcpy(TheMipData, MipData, MipMap.BulkData.GetBulkDataSize());
					}
					else
					{
						EPixelFormat PixelFormat = SubstanceData->Owner->Format;
						const uint32 BlockSizeX = GPixelFormats[PixelFormat].BlockSizeX;	//Block width in pixels
						const uint32 BlockSizeY = GPixelFormats[PixelFormat].BlockSizeY;	//Block height in pixels
						const uint32 BlockBytes = GPixelFormats[PixelFormat].BlockBytes;
						uint32 NumColumns = (MipMap.SizeX + BlockSizeX - 1) / BlockSizeX;	//Num-of columns in the source data (in blocks)
						uint32 NumRows = (MipMap.SizeY + BlockSizeY - 1) / BlockSizeY;	    //Num-of rows in the source data (in blocks)
						if (PixelFormat == PF_PVRTC2 || PixelFormat == PF_PVRTC4)
						{
							//PVRTC has minimum 2 blocks width and height
							NumColumns = FMath::Max<uint32>(NumColumns, 2);
							NumRows = FMath::Max<uint32>(NumRows, 2);
						}
						const uint32 SrcPitch = NumColumns * BlockBytes;					//Num-of bytes per row in the source data
						const uint32 EffectiveSize = BlockBytes*NumColumns*NumRows;

						//Copy the texture data.
						CopyTextureData2D(MipData,TheMipData,MipMap.SizeY,PixelFormat,SrcPitch,DestPitch);
					}

					MipMap.BulkData.Unlock();

					RHIUnlockTexture2D(SubstanceData->Resource->GetTexture2DRHI(), MipIndex, false);
				}

				delete SubstanceData;
			});
	}
}

//Note - The flags at the bottom might be able to be altered due to engine updates? Look into this
/** Create RHI sampler states. */
void FSubstanceTexture2DDynamicResource::CreateSamplerStates(float MipMapBias)
{
	//Create the sampler state RHI resource.
	FSamplerStateInitializerRHI SamplerStateInitializer
	(
		(ESamplerFilter)UDeviceProfileManager::Get().GetActiveProfile()->GetTextureLODSettings()->GetSamplerFilter(SubstanceOwner),
		SubstanceOwner->AddressX == TA_Wrap ? AM_Wrap : (SubstanceOwner->AddressX == TA_Clamp ? AM_Clamp : AM_Mirror),
		SubstanceOwner->AddressY == TA_Wrap ? AM_Wrap : (SubstanceOwner->AddressY == TA_Clamp ? AM_Clamp : AM_Mirror),
		AM_Wrap,
		MipMapBias
	);
	SamplerStateRHI = RHICreateSamplerState(SamplerStateInitializer);

	//Create a custom sampler state for using this texture in a deferred pass, where ddx / ddy are discontinuous
	FSamplerStateInitializerRHI DeferredPassSamplerStateInitializer
	(
		(ESamplerFilter)UDeviceProfileManager::Get().GetActiveProfile()->GetTextureLODSettings()->GetSamplerFilter(SubstanceOwner),
		SubstanceOwner->AddressX == TA_Wrap ? AM_Wrap : (SubstanceOwner->AddressX == TA_Clamp ? AM_Clamp : AM_Mirror),
		SubstanceOwner->AddressY == TA_Wrap ? AM_Wrap : (SubstanceOwner->AddressY == TA_Clamp ? AM_Clamp : AM_Mirror),
		AM_Wrap,
		MipMapBias,
		//Disable anisotropic filtering, since aniso doesn't respect MaxLOD
		1,
		0,
		//Prevent the less detailed mip levels from being used, which hides artifacts on silhouettes due to ddx / ddy being very large
		//This has the side effect that it increases minification aliasing on light functions
		2
	);

	DeferredPassSamplerStateRHI = RHICreateSamplerState(DeferredPassSamplerStateInitializer);
}

//Called when the resource is initialized. This is only called by the rendering thread.
void FSubstanceTexture2DDynamicResource::InitRHI()
{
	float MipMapBias =
		FMath::Clamp<float>(
			UTexture2D::GetGlobalMipMapLODBias() + ((SubstanceOwner->LODGroup == TEXTUREGROUP_UI) ?
				-SubstanceOwner->Mips.Num() :
				SubstanceOwner->LODBias),
			0,
			SubstanceOwner->Mips.Num());

	//Create the sampler state RHI resource.
	CreateSamplerStates(MipMapBias);

	uint32 Flags = 0;
	if (SubstanceOwner->bIsResolveTarget)
	{
		Flags |= TexCreate_ResolveTargetable;
		bIgnoreGammaConversions = true;		//Note, we're ignoring Owner->SRGB (it should be false).
	}

	if (SubstanceOwner->SRGB)
	{
		Flags |= TexCreate_SRGB;
	}
	else
	{
		bIgnoreGammaConversions = true;
		bSRGB = false;
	}

	if (SubstanceOwner->bNoTiling)
	{
		Flags |= TexCreate_NoTiling;
	}

#if PLATFORM_PS4
	Flags |= TexCreate_OfflineProcessed;
#endif

	FRHIResourceCreateInfo CreateInfo;
	Texture2DRHI = RHICreateTexture2D(GetSizeX(), GetSizeY(), SubstanceOwner->Format, SubstanceOwner->NumMips, 1, Flags, CreateInfo);
	TextureRHI = Texture2DRHI;
	RHIUpdateTextureReference(SubstanceOwner->TextureReference.TextureReferenceRHI, TextureRHI);
}

//Called when the resource is released. This is only called by the rendering thread.
void FSubstanceTexture2DDynamicResource::ReleaseRHI()
{
	RHIUpdateTextureReference(SubstanceOwner->TextureReference.TextureReferenceRHI, FTextureRHIParamRef());
	FTextureResource::ReleaseRHI();
	Texture2DRHI.SafeRelease();
}

//Returns the Texture2DRHI, which can be used for locking/unlocking the mips.
FTexture2DRHIRef FSubstanceTexture2DDynamicResource::GetTexture2DRHI()
{
	return Texture2DRHI;
}