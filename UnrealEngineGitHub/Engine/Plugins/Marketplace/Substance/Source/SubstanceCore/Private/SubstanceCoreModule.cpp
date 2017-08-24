// Copyright 2016 Allegorithmic Inc. All rights reserved.
// File: SubstanceCoreModule.cpp

#include "SubstanceCorePrivatePCH.h"
#include "SubstanceCoreHelpers.h"
#include "SubstanceCoreModule.h"

#include "SubstanceCoreClasses.h"
#include "SubstanceStyle.h"

#include "AssetRegistryModule.h"
#include "ModuleManager.h"
#include "Ticker.h"

#if SUBSTANCE_MEMORY_STAT
DEFINE_STAT(STAT_SubstanceCacheMemory);
DEFINE_STAT(STAT_SubstanceEngineMemory);
DEFINE_STAT(STAT_SubstanceMemory);
#endif

DEFINE_LOG_CATEGORY_STATIC(LogSubstanceCoreModule, Log, All);

//Resolve library file name depending on build platform
#if defined (SUBSTANCE_ENGINE_DYNAMIC)
#	if defined(SUBSTANCE_ENGINE_DEBUG)
#		define SUBSTANCE_LIB_CONFIG "Debug"
#	else
#		define SUBSTANCE_LIB_CONFIG "Release"
#	endif
#	if PLATFORM_WINDOWS
#		if PLATFORM_64BITS
#			if _MSC_VER < 1900
#				define SUBSTANCE_LIB_CPU_DYNAMIC_PATH	TEXT("DLLs/" SUBSTANCE_LIB_CONFIG "/Win64_2013/substance_sse2_blend.dll")
#				define SUBSTANCE_LIB_GPU_DYNAMIC_PATH	TEXT("DLLs/" SUBSTANCE_LIB_CONFIG "/Win64_2013/substance_d3d11pc_blend.dll")
#			else
#				define SUBSTANCE_LIB_CPU_DYNAMIC_PATH	TEXT("DLLs/" SUBSTANCE_LIB_CONFIG "/Win64/substance_sse2_blend.dll")
#				define SUBSTANCE_LIB_GPU_DYNAMIC_PATH	TEXT("DLLs/" SUBSTANCE_LIB_CONFIG "/Win64/substance_d3d11pc_blend.dll")
#			endif
#		else
#			error Unsupported platform for dynamic substance loading
#		endif
#   elif PLATFORM_MAC
#       define SUBSTANCE_LIB_CPU_DYNAMIC_PATH TEXT("DLLs/" SUBSTANCE_LIB_CONFIG "/Mac/libsubstance_sse2_blend.dylib")
#       define SUBSTANCE_LIB_GPU_DYNAMIC_PATH TEXT("DLLs/" SUBSTANCE_LIB_CONFIG "/Mac/libsubstance_ogl2_blend.dylib")
#   elif PLATFORM_LINUX
#		define SUBSTANCE_LIB_CPU_DYNAMIC_PATH TEXT("DLLs/" SUBSTANCE_LIB_CONFIG "/Linux/libsubstance_sse2_blend.so")
#		define SUBSTANCE_LIB_GPU_DYNAMIC_PATH TEXT("DLLs/" SUBSTANCE_LIB_CONFIG "/Linux/libsubstance_ogl2_blend.so")
#   else
#       error Unsupported platform for dynamic substance loading
#	endif
#endif

namespace
{
	static FWorldDelegates::FWorldInitializationEvent::FDelegate OnWorldInitDelegate;
	static FDelegateHandle OnWorldInitDelegateHandle;

	static FWorldDelegates::FOnLevelChanged::FDelegate OnLevelAddedDelegate;
	static FDelegateHandle OnLevelAddedDelegateHandle;
}

void FSubstanceCoreModule::StartupModule()
{
	void* libraryPtr = nullptr;

#if defined (SUBSTANCE_ENGINE_DYNAMIC)
	const TCHAR* libraryFileName =
		(GetDefault<USubstanceSettings>()->SubstanceEngine == ESubstanceEngineType::SET_CPU)
		? SUBSTANCE_LIB_CPU_DYNAMIC_PATH
		: SUBSTANCE_LIB_GPU_DYNAMIC_PATH;

	//The plugin can exist in a few different places, so lets try each one in turn
	FString prefixPaths[] = {
		FPaths::Combine(*FPaths::EnginePluginsDir(), TEXT("Runtime/")),
		FPaths::Combine(*FPaths::GamePluginsDir(), TEXT("Runtime/")),
		FPaths::Combine(*FPaths::EnginePluginsDir(), TEXT("Marketplace/")),
	};

	size_t numPaths = sizeof(prefixPaths) / sizeof(FString);
	for (size_t i = 0; i < numPaths; i++)
	{
		FString libraryPath = FPaths::Combine(*prefixPaths[i], TEXT("Substance/"), libraryFileName);
		if (FPaths::FileExists(libraryPath))
		{
			if (void* pLibraryHandle = FPlatformProcess::GetDllHandle(*libraryPath))
			{
				libraryPtr = pLibraryHandle;
			}
		}
	}

	if (libraryPtr == nullptr)
	{
		UE_LOG(LogSubstanceCoreModule, Fatal, TEXT("Unable to load Substance Library."));
	}
#endif

	if (GetDefault<USubstanceSettings>()->SubstanceEngine == ESubstanceEngineType::SET_CPU)
	{
		UE_LOG(LogSubstanceCoreModule, Log, TEXT("Substance [CPU] Engine Loaded, Max Texture Size = 2048"));
	}
	else
	{
		UE_LOG(LogSubstanceCoreModule, Log, TEXT("Substance [GPU] Engine Loaded, Max Texture Size = 4096"));
	}

	//Register tick function
	if (!IsRunningDedicatedServer())
	{
		//Init substance core objects
		Substance::Helpers::SetupSubstance(libraryPtr);

		TickDelegate = FTickerDelegate::CreateRaw(this, &FSubstanceCoreModule::Tick);
		FTicker::GetCoreTicker().AddTicker(TickDelegate);

		RegisterSettings();

		::OnWorldInitDelegate = FWorldDelegates::FWorldInitializationEvent::FDelegate::CreateStatic(&FSubstanceCoreModule::OnWorldInitialized);
		::OnWorldInitDelegateHandle = FWorldDelegates::OnPostWorldInitialization.Add(::OnWorldInitDelegate);

		::OnLevelAddedDelegate = FWorldDelegates::FOnLevelChanged::FDelegate::CreateStatic(&FSubstanceCoreModule::OnLevelAdded);
		::OnLevelAddedDelegateHandle = FWorldDelegates::LevelAddedToWorld.Add(::OnLevelAddedDelegate);

#if WITH_EDITOR
		FEditorDelegates::BeginPIE.AddRaw(this, &FSubstanceCoreModule::OnBeginPIE);
		FEditorDelegates::EndPIE.AddRaw(this, &FSubstanceCoreModule::OnEndPIE);
		FSubstanceStyle::Initialize();
#endif //WITH_EDITOR

	}

	PIE = false;
}

void FSubstanceCoreModule::ShutdownModule()
{
	FSubstanceStyle::Shutdown();
	FWorldDelegates::OnPostWorldInitialization.Remove(::OnWorldInitDelegateHandle);
	FWorldDelegates::LevelAddedToWorld.Remove(::OnLevelAddedDelegateHandle);

	UnregisterSettings();

	Substance::Helpers::TearDownSubstance();
}

bool FSubstanceCoreModule::Tick(float DeltaTime)
{
	Substance::Helpers::Tick();

	return true;
}

unsigned int FSubstanceCoreModule::GetMaxOutputTextureSize() const
{
	if (GetDefault<USubstanceSettings>()->SubstanceEngine == ESubstanceEngineType::SET_GPU)
		return 4096;
	else
		return 2048;
}

void FSubstanceCoreModule::OnWorldInitialized(UWorld* World, const UWorld::InitializationValues IVS)
{
	Substance::Helpers::OnWorldInitialized();
}

void FSubstanceCoreModule::OnLevelAdded(ULevel* Level, UWorld* World)
{
	Substance::Helpers::OnLevelAdded();
}

#if WITH_EDITOR
void FSubstanceCoreModule::OnBeginPIE(const bool bIsSimulating)
{
	PIE = true;
}

void FSubstanceCoreModule::OnEndPIE(const bool bIsSimulating)
{
	PIE = false;
}
#endif

IMPLEMENT_MODULE(FSubstanceCoreModule, SubstanceCore);