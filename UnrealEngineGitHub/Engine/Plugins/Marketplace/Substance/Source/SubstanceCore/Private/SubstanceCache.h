// Copyright 2016 Allegorithmic Inc. All rights reserved.
// File: SubstanceCache.h

#pragma once

/** Forward Declare */
class FArchive;
class USubstanceTexture2D;
class OutputInstance;
class GraphInstance;

namespace Substance
{
	class SubstanceCache
	{
	public:
		/** Singleton implementation to create - return class */
		static TSharedPtr<SubstanceCache> Get()
		{
			if (!SbsCache.IsValid())
				SbsCache = MakeShareable(new SubstanceCache);

			return SbsCache;
		}

		/** Cleans up class instance */
		static void Shutdown()
		{
			if (!SbsCache.IsValid())
				SbsCache.Reset();
		}

		/** Checks to see if graph is stored in cache and can be read */
		bool CanReadFromCache(SubstanceAir::GraphInstance* graph);

		/** Load a graph instance from cache */
		bool ReadFromCache(SubstanceAir::GraphInstance* graph);

		/** Save an output instance to cache */
		void CacheOutput(SubstanceAir::OutputInstance* Output, const SubstanceTexture& result);

	private:
		/** Get the path to the cache working directory path */
		FString GetPathForGuid(const FGuid& guid) const;

		/** Serializes frameworks SubstanceTexture objects */
		bool SerializeTexture(FArchive& Ar, SubstanceTexture& result) const;

		/** Singleton class instance */
		static TSharedPtr<SubstanceCache> SbsCache;
	};
}
