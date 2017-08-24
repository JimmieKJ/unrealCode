// Copyright 2016 Allegorithmic Inc. All rights reserved.
// File: SubstanceSettings.cpp

#include "SubstanceCorePrivatePCH.h"
#include "SubstanceInstanceFactory.h"
#include "SubstanceSettings.h"

USubstanceSettings::USubstanceSettings(const FObjectInitializer& PCIP)
	: Super(PCIP)
	, MemoryBudgetMb(512)
	, CPUCores(32)
	, AsyncLoadMipClip(3)
	, MaxAsyncSubstancesRenderedPerFrame(10)
	, SubstanceEngine(SET_CPU)
{
}