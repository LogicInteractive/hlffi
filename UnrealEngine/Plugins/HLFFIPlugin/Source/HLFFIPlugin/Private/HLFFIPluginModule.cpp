// Copyright LogicInteractive. All Rights Reserved.

#include "HLFFIPluginModule.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"

// Define HLFFI_IMPLEMENTATION in this compilation unit only
#define HLFFI_IMPLEMENTATION
#include "hlffi.h"

DEFINE_LOG_CATEGORY(LogHLFFI);

#define LOCTEXT_NAMESPACE "FHLFFIPluginModule"

void FHLFFIPluginModule::StartupModule()
{
	UE_LOG(LogHLFFI, Log, TEXT("HLFFI Plugin starting up..."));

	// Load the HashLink DLL
	if (!LoadHashLinkDll())
	{
		UE_LOG(LogHLFFI, Error, TEXT("Failed to load HashLink DLL. HLFFI functionality will not be available."));
		return;
	}

	UE_LOG(LogHLFFI, Log, TEXT("HLFFI Plugin started successfully. HLFFI Version: %s"),
		ANSI_TO_TCHAR(hlffi_get_version()));
}

void FHLFFIPluginModule::ShutdownModule()
{
	UE_LOG(LogHLFFI, Log, TEXT("HLFFI Plugin shutting down..."));

	UnloadHashLinkDll();

	UE_LOG(LogHLFFI, Log, TEXT("HLFFI Plugin shutdown complete."));
}

FHLFFIPluginModule& FHLFFIPluginModule::Get()
{
	return FModuleManager::LoadModuleChecked<FHLFFIPluginModule>("HLFFIPlugin");
}

bool FHLFFIPluginModule::IsAvailable()
{
	return FModuleManager::Get().IsModuleLoaded("HLFFIPlugin");
}

bool FHLFFIPluginModule::LoadHashLinkDll()
{
#if PLATFORM_WINDOWS
	// Try multiple locations for the DLL
	TArray<FString> SearchPaths;

	// 1. Plugin ThirdParty folder
	FString PluginDir = FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("HLFFIPlugin"));
	SearchPaths.Add(FPaths::Combine(PluginDir, TEXT("ThirdParty/hashlink/lib/Win64")));

	// 2. Project Binaries folder
	SearchPaths.Add(FPaths::Combine(FPaths::ProjectDir(), TEXT("Binaries/Win64")));

	// 3. Engine Binaries folder
	SearchPaths.Add(FPaths::EngineDir() / TEXT("Binaries/Win64"));

	FString DllName = TEXT("libhl.dll");

	for (const FString& SearchPath : SearchPaths)
	{
		FString DllPath = FPaths::Combine(SearchPath, DllName);

		if (FPaths::FileExists(DllPath))
		{
			// Push the directory to DLL search path
			FPlatformProcess::PushDllDirectory(*SearchPath);

			HashLinkDllHandle = FPlatformProcess::GetDllHandle(*DllPath);

			FPlatformProcess::PopDllDirectory(*SearchPath);

			if (HashLinkDllHandle)
			{
				UE_LOG(LogHLFFI, Log, TEXT("Loaded HashLink DLL from: %s"), *DllPath);
				return true;
			}
		}
	}

	UE_LOG(LogHLFFI, Error, TEXT("Could not find libhl.dll. Searched paths:"));
	for (const FString& SearchPath : SearchPaths)
	{
		UE_LOG(LogHLFFI, Error, TEXT("  - %s"), *SearchPath);
	}

	return false;
#else
	UE_LOG(LogHLFFI, Warning, TEXT("HLFFI Plugin currently only supports Windows platform."));
	return false;
#endif
}

void FHLFFIPluginModule::UnloadHashLinkDll()
{
	if (HashLinkDllHandle)
	{
		FPlatformProcess::FreeDllHandle(HashLinkDllHandle);
		HashLinkDllHandle = nullptr;
		UE_LOG(LogHLFFI, Log, TEXT("Unloaded HashLink DLL."));
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FHLFFIPluginModule, HLFFIPlugin)
