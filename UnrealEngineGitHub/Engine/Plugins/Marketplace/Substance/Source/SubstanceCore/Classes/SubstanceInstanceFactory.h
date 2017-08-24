// Copyright 2016 Allegorithmic Inc. All rights reserved.
// File: SubstanceInstanceFactory.h

#pragma once
#include "Engine.h"
#include "substance/framework/framework.h"
#include "SubstanceInstanceFactory.generated.h"

/** Forward Declare */
class PackageDesc;
class USubstanceGraphInstance;
class USubstanceInstanceFactory;
struct LegacyPackage;

/** User data referenced by the framework */
struct PackageDescData
{
	USubstanceInstanceFactory* ParentFactory;
};

UENUM(BlueprintType)
enum ESubstanceGenerationMode
{
	SGM_PlatformDefault = 0,
	SGM_Baked = 1,
	SGM_OnLoadSync = 2,
	SGM_OnLoadSyncAndCache = 3,
	SGM_OnLoadAsync = 4,
	SGM_OnLoadAsyncAndCache = 5,
	SGM_MAX = 6
};

UCLASS(hideCategories = Object, MinimalAPI, BlueprintType)
class USubstanceInstanceFactory : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	/** Framework container that stores all of the description data for owned graphs */
	SubstanceAir::PackageDesc* SubstancePackage;

	/** Data references stored within the framework */
	PackageDescData mPackageUserData;

	/** Used for reimporting sbs files within editor */
	UPROPERTY()
	FString	RelativeSourceFilePath;

	/** Used for reimporting sbs files within editor */
	UPROPERTY()
	FString	AbsoluteSourceFilePath;

	/** Used to store when the file was created */
	UPROPERTY()
	FString	SourceFileTimestamp;

	/** Flag for the format used to save asset data */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Substance")
	TEnumAsByte<ESubstanceGenerationMode> GenerationMode;

	/** Serializes the assets that were created and saved post framework refactor */
	void SerializeCurrent(FArchive& Ar);

	/** Serializes assets that were created before the framework refactor */
	void SerializeLegacy(FArchive& Ar);

	/** Begin UObject interface */
	virtual void Serialize(FArchive& Ar) override;
	virtual void BeginDestroy() override;
	virtual void PostDuplicate(bool bDuplicateForPIE) override;
	virtual void PostLoad() override;
	/** End UObject interface */

	/** Returns the resource size based on how we are saving the data */
	virtual void GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) override;

	/** Get the generation mode for this package */
	ESubstanceGenerationMode GetGenerationMode() const;

	/** Whether or not the output instances should be cached */
	bool ShouldCacheOutput() const;

	/** Increases the count of graph instances relying on this factory - Used for cleanup */
	void RegisterGraphInstance(USubstanceGraphInstance* Graph);

	/** Decreases the count of graph instances relying on this factory - Used for cleanup */
	SUBSTANCECORE_API void UnregisterGraphInstance(USubstanceGraphInstance* Graph);

	/** Returns the array of current associated graphs */
	SUBSTANCECORE_API const TArray<USubstanceGraphInstance*> GetGraphInstances();

	/** Returns a list of strings of all of the graph packages from the current graph instances */
	SUBSTANCECORE_API void ClearReferencingObjects();

private:

	/** Stores references to the instances that are created with this factory */
	TArray<USubstanceGraphInstance*> GraphInstances;
};
