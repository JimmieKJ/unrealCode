// Copyright 2016 Allegorithmic Inc. All rights reserved.
// File: SubstanceEditorModule.h

#pragma once
#include "UnrealEd.h"
#include "ModuleInterface.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Toolkits/IToolkit.h"
#include "ISubstanceEditor.h"

namespace SubstanceEditorModule
{
	extern const FName SubstanceEditorAppIdentifier;
}

class ISubstanceEditorModule : public IModuleInterface, public IHasMenuExtensibility, public IHasToolBarExtensibility
{
public:
	/** Creates a new Font editor */
	virtual TSharedRef<ISubstanceEditor> CreateSubstanceEditor(const TSharedPtr< IToolkitHost >& InitToolkitHost, USubstanceGraphInstance* Font) = 0;
};
