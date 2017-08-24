// Copyright 2016 Allegorithmic Inc. All rights reserved.
// File: ISubstanceCore.h

#pragma once
#include "ModuleInterface.h"

/** The public interface of the SubstanceCore module */
class ISubstanceCore : public IModuleInterface
{
public:
	/** Returns the Maximum Output size for Substance Texture 2D */
	virtual unsigned int GetMaxOutputTextureSize() const = 0;
};
