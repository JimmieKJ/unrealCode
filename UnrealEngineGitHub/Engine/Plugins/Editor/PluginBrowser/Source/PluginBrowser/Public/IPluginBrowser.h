// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleManager.h"
#include "Editor/UnrealEd/Public/Features/IPluginsEditorFeature.h"

/**
 * The public interface to this module
 */
class IPluginBrowser : public IModuleInterface, public IPluginsEditorFeature
{

public:

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline IPluginBrowser& Get()
	{
		return FModuleManager::LoadModuleChecked< IPluginBrowser >( "PluginBrowser" );
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded( "PluginBrowser" );
	}
};

