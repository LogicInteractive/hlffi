// Copyright LogicInteractive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHLFFI, Log, All);

/**
 * HLFFI Plugin Module
 *
 * Manages the HashLink FFI integration with Unreal Engine.
 * Handles DLL loading and provides access to the HLFFI subsystem.
 */
class FHLFFIPluginModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/**
	 * Get the module instance
	 * @return Reference to the HLFFI Plugin module
	 */
	static FHLFFIPluginModule& Get();

	/**
	 * Check if the module is loaded and available
	 * @return True if the module is loaded
	 */
	static bool IsAvailable();

private:
	/** Handle to the HashLink DLL */
	void* HashLinkDllHandle = nullptr;

	/** Load the HashLink DLL */
	bool LoadHashLinkDll();

	/** Unload the HashLink DLL */
	void UnloadHashLinkDll();
};
